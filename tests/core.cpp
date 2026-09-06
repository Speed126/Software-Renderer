// SPDX-License-Identifier: MIT
#include "renderer/pipeline.h"
#include "renderer/rasterizer.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace renderer;
void check(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
bool near(float a, float b) {
    return std::abs(a - b) < 1e-5f;
}
int main() {
    try {
        check(near(dot({1, 2, 3}, {4, 5, 6}), 32), "dot product");
        check(cross({1, 0, 0}, {0, 1, 0}).z == 1, "cross handedness");
        check(near(length(normalized({3, 4, 0})), 1), "normalization");
        check(length(normalized({})) == 0, "zero normalization");
        const std::array<Vec2, 3> points{{{0, 0}, {4, 0}, {0, 4}}};
        auto w = barycentric(points, {1, 1});
        check(w && near((*w)[0], 0.5f) && near((*w)[1], 0.25f), "barycentrics");
        check((*barycentric(points, {5, 5}))[0] < 0, "outside triangle");
        check(!barycentric({{{0, 0}, {1, 1}, {2, 2}}}, {0, 0}), "degenerate triangle");
        Framebuffer a(8, 8), b(8, 8);
        check(!a.write(-1, 0, 0, {}) && !a.write(8, 0, 0, {}), "bounds");
        check(!a.write(0, 0, -1, {}) && !a.write(0, 0, NAN, {}), "invalid depth");
        const std::array<Vec3, 3> front{{{-4, -4, 0.2f}, {12, -4, 0.2f}, {-4, 12, 0.2f}}};
        auto back = front;
        for (auto& v : back)
            v.z = 0.8f;
        rasterize(a, front, {255, 0, 0});
        rasterize(a, back, {0, 255, 0});
        rasterize(b, back, {0, 255, 0});
        rasterize(b, front, {255, 0, 0});
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x)
                check(a.color(x, y) == b.color(x, y), "draw order");
        check(a.color(1, 1) == Color{255, 0, 0} && near(a.depth(1, 1), 0.2f),
              "near triangle visible");
        Framebuffer culled(8, 8);
        auto reversed = front;
        std::swap(reversed[1], reversed[2]);
        rasterize(culled, reversed, {255, 0, 0});
        check(std::isinf(culled.depth(1, 1)), "back-face culling");
        rasterize(culled, {{{1, 1, 0}, {2, 2, 0}, {3, 3, 0}}}, {255, 0, 0});
        check(std::isinf(culled.depth(1, 1)), "degenerate writes nothing");
        const Vec4 v{1, 2, 3, 1};
        check((Mat4::identity() * v).y == 2, "identity");
        check((translation({2, 3, 4}) * v).x == 3, "translation");
        check((scaling({2, 3, 4}) * v).z == 12, "scaling");
        const auto composed = translation({1, 0, 0}) * scaling({2, 2, 2}) * v;
        check(composed.x == 3 && composed.y == 4, "matrix multiplication order");
        check(near((rotation_y(1.57079632679f) * Vec4{1, 0, 0, 1}).z, -1), "rotation");
        const auto projection = perspective(1.57079632679f, 1, 1, 10);
        const auto close = projection * Vec4{1, 0, -1, 1}, far = projection * Vec4{1, 0, -10, 1};
        check(near(close.z / close.w, -1) && near(far.z / far.w, 1), "near/far mapping");
        check(close.x / close.w > far.x / far.w, "perspective foreshortening");
        const auto middle = viewport({0, 0, 0, 1}, 100, 80);
        check(middle.x == 50 && middle.y == 40 && middle.z == 0.5f, "viewport");
        check(near((look_at({0, 0, 3}, {0, 0, 0}, {0, 1, 0}) * Vec4{0, 0, 0, 1}).z, -3),
              "view transform");
        const auto clipped = clip_triangle(
            {{{{-0.5f, 0, -2, 1}, {}, {}}, {{0.5f, 0, 0, 1}, {}, {}}, {{0, 0.5f, 0, 1}, {}, {}}}});
        check(clipped.size() == 4, "near plane creates quad");
        for (auto p : clipped)
            check(p.position.z >= -p.position.w && p.position.z <= p.position.w,
                  "clipped depth bounds");
        check(clip_triangle(
                  {{{{0, 0, 2, 1}, {}, {}}, {{1, 0, 2, 1}, {}, {}}, {{0, 1, 2, 1}, {}, {}}}})
                  .empty(),
              "fully clipped");
        const auto parse = [](const std::string& text) {
            std::istringstream in(text);
            return Model::parse(in, "fixture");
        };
        const std::string vertices = "v 0 0 0\nv 1 0 0\nv 0 1 0\n";
        check(parse(vertices + "f 1 2 3\n").faces.size() == 1, "position-only OBJ");
        check(parse(vertices + "f -3 -2 -1 # comment\n").faces[0][2].position == 2,
              "negative OBJ indices");
        const auto attributed =
            parse(vertices + "vt 0 0\nvt 1 0\nvt 0 1\nvn 0 0 1\nf 1/1/1 2/2/1 3/3/1\n");
        check(attributed.faces[0][1].uv == 1 && attributed.normals[0].z == 1, "OBJ attributes");
        check(parse(vertices + "vn 0 0 1\nf 1//1 2//1 3//1\n").faces[0][0].uv == -1, "missing UV");
        for (const std::string bad :
             {"f 0 2 3\n", "f 1 2 4\n", "f 1 2\n", "f 1 2 3 1\n", "f 1/1 2/1 3/1\n"}) {
            bool failed = false;
            try {
                parse(vertices + bad);
            } catch (const std::runtime_error& e) {
                failed = std::string(e.what()).find("fixture:4:") != std::string::npos;
            }
            check(failed, "malformed OBJ must report line");
        }
        std::istringstream ppm("P3\n# colors\n2 2\n255\n255 0 0  0 255 0\n0 0 255  255 255 255\n");
        const auto texture = Texture::parse(ppm);
        check(texture.sample({0.1f, 0.1f}) == Color{0, 0, 255}, "UV bottom origin");
        check(texture.sample({0.1f, 0.6f}) == Color{255, 0, 0}, "UV top row");
        check(texture.sample({-0.9f, 1.1f}) == Color{0, 0, 255}, "UV repeat");
        check(lambert({255, 255, 255}, {0, 0, 1}, {0, 0, 1}).r >
                  lambert({255, 255, 255}, {1, 0, 0}, {0, 0, 1}).r,
              "Lambert direction");
        std::array<ScreenVertex, 3> varying{};
        varying[0].inverse_w = 1;
        varying[1].inverse_w = 0.5f;
        varying[2].inverse_w = 0.25f;
        const auto corrected = perspective_weights({0.25f, 0.25f, 0.5f}, varying);
        check(near(corrected[0], 0.5f) && near(corrected[1], 0.25f) && near(corrected[2], 0.25f),
              "perspective attributes");
        Framebuffer fallback(32, 32);
        render(parse(vertices + "f 1 2 3\n"), fallback);
        bool visible = false;
        for (int y = 0; y < 32; ++y)
            for (int x = 0; x < 32; ++x)
                visible = visible || std::isfinite(fallback.depth(x, y));
        check(visible, "missing attributes render fallback");
        std::cout << "Core checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
