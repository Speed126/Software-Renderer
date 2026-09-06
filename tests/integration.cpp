// SPDX-License-Identifier: MIT
#include "renderer/pipeline.h"
#include "renderer/rasterizer.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
using namespace renderer;
namespace {
void check(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
template <class F> void rejects(F operation, const char* message) {
    bool rejected = false;
    try {
        operation();
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected, message);
}
Model parse(const std::string& text) {
    std::istringstream stream(text);
    return Model::parse(stream);
}
void coverage() {
    Framebuffer a(8, 8), b(8, 8);
    const std::array<Vec3, 3> lower{{{0, 0, 0.4f}, {8, 0, 0.4f}, {8, 8, 0.4f}}};
    const std::array<Vec3, 3> upper{{{0, 0, 0.4f}, {8, 8, 0.4f}, {0, 8, 0.4f}}};
    rasterize(a, lower, {255, 0, 0});
    rasterize(a, upper, {0, 255, 0});
    rasterize(b, upper, {0, 255, 0});
    rasterize(b, lower, {255, 0, 0});
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) {
            check(std::isfinite(a.depth(x, y)), "shared edge leaves no holes");
            check(a.color(x, y) == b.color(x, y), "shared edge belongs to exactly one triangle");
        }
    rejects([&] { a.color(8, 0); }, "out-of-bounds read must throw");
    rejects([] { Framebuffer invalid(-1, 8); }, "invalid allocation");
    auto reversed = lower;
    std::swap(reversed[1], reversed[2]);
    Framebuffer two_sided(8, 8);
    rasterize(two_sided, reversed, {255, 0, 0}, false);
    check(std::isfinite(two_sided.depth(5, 1)), "culling can be disabled");
    Framebuffer offscreen(8, 8);
    rasterize(offscreen, {{{1e10f, 1e10f, 0}, {1e10f + 1024, 1e10f, 0}, {1e10f, 1e10f + 1024, 0}}},
              {255, 0, 0});
    check(std::isinf(offscreen.depth(0, 0)), "extreme offscreen coordinates");
    rasterize(offscreen, {{{NAN, 0, 0}, {1, 0, 0}, {0, 1, 0}}}, {255, 0, 0});
    check(std::isinf(offscreen.depth(0, 0)), "nonfinite coordinates");
}
void interpolation() {
    std::istringstream input("P3 2 1 255 255 0 0 0 255 0");
    const Texture texture = Texture::parse(input);
    const std::array<ScreenVertex, 3> t{{{{0, 0, 0.2f}, 1, {0, 0.5f}, {0, 0, 1}},
                                         {{8, 0, 0.8f}, 0.1f, {1, 0.5f}, {0, 0, 1}},
                                         {{0, 8, 0.8f}, 0.1f, {1, 0.5f}, {0, 0, 1}}}};
    Surface surface;
    surface.texture = &texture;
    surface.lit = false;
    Framebuffer target(8, 8);
    rasterize(target, t, surface);
    // At (2.5,2.5), affine u=.625 (green), corrected u=1/7 (red).
    check(target.color(2, 2) == Color{255, 0, 0}, "rasterizer uses perspective-correct UVs");
    check(std::abs(target.depth(2, 2) - 0.575f) < 1e-5f, "depth stays screen-affine");
    const std::array<ClipVertex, 3> triangle{{{{-0.5f, 0, -2, 1}, {0, 0}, {0, 0, 1}},
                                              {{0.5f, 0, 0, 1}, {1, 0}, {0, 0, 1}},
                                              {{0, 0.5f, 0, 1}, {0, 1}, {0, 0, 1}}}};
    const auto polygon = clip_triangle(triangle);
    int intersections = 0;
    for (const auto& v : polygon)
        if (v.position.z == -1) {
            ++intersections;
            check(std::abs(v.uv.x + v.uv.y - 0.5f) < 1e-5f, "clipping interpolates attributes");
        }
    check(intersections == 2, "two near intersections");
    for (int axis = 0; axis < 3; ++axis)
        for (float sign : {-1.0f, 1.0f}) {
            auto outside = triangle;
            for (auto& v : outside) {
                v.position = {0, 0, 0, 1};
                if (axis == 0)
                    v.position.x = 2 * sign;
                if (axis == 1)
                    v.position.y = 2 * sign;
                if (axis == 2)
                    v.position.z = 2 * sign;
            }
            check(clip_triangle(outside).empty(), "all six clip planes reject");
        }
}
void parsing() {
    const std::string base = "v 0 0 0\nv 1 0 0\nv 0 1 0\n";
    check(parse(base + "vt 0 0 0\nf 1/1 2/1 3/1\n").faces[0][0].uv == 0, "optional third vt");
    check(parse(base + "vt 0 0\nvn 0 0 2\nf -3/-1/-1 -2/-1/-1 -1/-1/-1\n").normals[0].z == 1,
          "negative attribute indices");
    for (const std::string face : {"f 1// 2// 3//", "f 1/a 2/a 3/a", "f -4 2 3", "f 1/1/1/1 2 3"})
        rejects([&] { parse(base + face); }, "invalid face tokens");
    for (const std::string bad : {"", "v nan 0 0", "v 0 0", "vn 0 0 0", "vt 0 junk"})
        rejects([&] { parse(bad); }, "invalid model data");
    for (const std::string bad :
         {"P6 1 1 255", "P3 0 1 255", "P3 1 1 0", "P3 1 1 255 256 0 0", "P3 1 1 255 0 0"})
        rejects(
            [&] {
                std::istringstream in(bad);
                Texture::parse(in);
            },
            "invalid PPM");
    rejects([] { look_at({0, 1, 0}, {0, 0, 0}, {0, 1, 0}); }, "parallel camera up");
    rejects([] { perspective(0, 1, 1, 10); }, "invalid field of view");
    rejects([] { viewport({0, 0, 0, 0}, 8, 8); }, "zero divide");
}
void end_to_end() {
    auto model = Model::load("assets/orbital.obj");
    const auto texture = Texture::checker();
    RenderOptions options;
    options.texture = &texture;
    Framebuffer first(96, 80), reversed(96, 80), transformed(96, 80);
    render(model, first, options);
    std::reverse(model.faces.begin(), model.faces.end());
    render(model, reversed, options);
    // Auto-fit must remove arbitrary translation and uniform scale.
    for (auto& p : model.positions)
        p = p * 7 + Vec3{10, -20, 30};
    render(model, transformed, options);
    int visible = 0, shifted = 0;
    for (int y = 0; y < 80; ++y)
        for (int x = 0; x < 96; ++x) {
            check(first.color(x, y) == reversed.color(x, y), "OBJ draw order invariance");
            if (std::isfinite(first.depth(x, y)))
                ++visible;
            if (first.color(x, y) != transformed.color(x, y))
                ++shifted;
        }
    check(visible > 1000, "smoke image contains substantial geometry");
    check(shifted < 30, "fitting is translation/scale invariant within rounding tolerance");
    first.save("integration.tga");
    std::ifstream input("integration.tga", std::ios::binary);
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)), {});
    check(bytes.size() == 18 + 96 * 80 * 3 + 26, "complete uncompressed TGA byte count");
    check(bytes[2] == 2 && bytes[12] == 96 && bytes[14] == 80 && bytes[16] == 24 && bytes[17] == 32,
          "TGA header");
    for (int y = 0; y < 80; ++y)
        for (int x = 0; x < 96; ++x) {
            const auto offset = static_cast<std::size_t>(18 + 3 * (y * 96 + x));
            const Color expected = first.color(x, 79 - y);
            check(bytes[offset] == expected.b && bytes[offset + 1] == expected.g &&
                      bytes[offset + 2] == expected.r,
                  "TGA BGR and top origin");
        }
}
} // namespace
int main() {
    try {
        coverage();
        interpolation();
        parsing();
        end_to_end();
        std::cout << "Integration checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
