// SPDX-License-Identifier: MIT
#include "renderer/rasterizer.h"
#include <algorithm>
#include <cmath>
namespace renderer {
float edge(Vec2 a, Vec2 b, Vec2 p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}
std::optional<std::array<float, 3>> barycentric(const std::array<Vec2, 3>& t, Vec2 p) {
    const float area = edge(t[0], t[1], t[2]);
    if (!std::isfinite(area) || std::abs(area) < 1e-8f)
        return std::nullopt;
    return std::array<float, 3>{edge(t[1], t[2], p) / area, edge(t[2], t[0], p) / area,
                                edge(t[0], t[1], p) / area};
}
void rasterize(Framebuffer& target, const std::array<ScreenVertex, 3>& input,
               const Surface& surface, bool cull) {
    auto t = input;
    for (auto v : t)
        if (!finite(v.position) || !std::isfinite(v.inverse_w) || v.inverse_w <= 0)
            return;
    std::array<Vec2, 3> p{{{t[0].position.x, t[0].position.y},
                           {t[1].position.x, t[1].position.y},
                           {t[2].position.x, t[2].position.y}}};
    float area = edge(p[0], p[1], p[2]);
    if (!std::isfinite(area) || std::abs(area) < 1e-8f || (cull && area <= 0))
        return;
    if (area < 0) {
        std::swap(t[1], t[2]);
        std::swap(p[1], p[2]);
    }
    // Clamp in floating point before converting: even very distant triangles are safe.
    const int left = static_cast<int>(std::clamp(std::floor(std::min({p[0].x, p[1].x, p[2].x})),
                                                 0.0f, static_cast<float>(target.width())));
    const int right = static_cast<int>(std::clamp(std::ceil(std::max({p[0].x, p[1].x, p[2].x})),
                                                  0.0f, static_cast<float>(target.width())));
    const int bottom = static_cast<int>(std::clamp(std::floor(std::min({p[0].y, p[1].y, p[2].y})),
                                                   0.0f, static_cast<float>(target.height())));
    const int top = static_cast<int>(std::clamp(std::ceil(std::max({p[0].y, p[1].y, p[2].y})), 0.0f,
                                                static_cast<float>(target.height())));
    const auto inclusive = [](Vec2 a, Vec2 b) { return b.y > a.y || (b.y == a.y && b.x < a.x); };
    for (int y = bottom; y < top; ++y)
        for (int x = left; x < right; ++x) {
            const Vec2 sample{static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f};
            bool inside = true;
            for (int i = 0; i < 3; ++i) {
                const Vec2 a = p[static_cast<std::size_t>(i)],
                           b = p[static_cast<std::size_t>((i + 1) % 3)];
                const float e = edge(a, b, sample);
                if (e < 0 || (e == 0 && !inclusive(a, b)))
                    inside = false;
            }
            if (!inside)
                continue;
            const auto w = *barycentric(p, sample);
            const auto q = perspective_weights(w, t);
            const Vec2 uv = t[0].uv * q[0] + t[1].uv * q[1] + t[2].uv * q[2];
            const Vec3 normal = t[0].normal * q[0] + t[1].normal * q[1] + t[2].normal * q[2];
            const Color albedo = surface.texture ? surface.texture->sample(uv) : surface.albedo;
            const Color color = surface.lit ? lambert(albedo, normal, surface.light) : albedo;
            // NDC depth is already affine in screen space; do not apply q to z.
            target.write(x, y,
                         w[0] * t[0].position.z + w[1] * t[1].position.z + w[2] * t[2].position.z,
                         color);
        }
}
} // namespace renderer

namespace renderer {
std::array<float, 3> perspective_weights(std::array<float, 3> w,
                                         const std::array<ScreenVertex, 3>& vertices) {
    float sum = 0;
    for (std::size_t i = 0; i < 3; ++i) {
        w[i] *= vertices[i].inverse_w;
        sum += w[i];
    }
    for (float& value : w)
        value /= sum;
    return w;
}
void rasterize(Framebuffer& target, const std::array<Vec3, 3>& triangle, Color color, bool cull) {
    std::array<ScreenVertex, 3> vertices{};
    for (std::size_t i = 0; i < 3; ++i)
        vertices[i].position = triangle[i];
    Surface surface;
    surface.albedo = color;
    surface.lit = false;
    rasterize(target, vertices, surface, cull);
}
} // namespace renderer
