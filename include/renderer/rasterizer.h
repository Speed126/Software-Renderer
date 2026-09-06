// SPDX-License-Identifier: MIT
#pragma once
#include "renderer/framebuffer.h"
#include "renderer/math.h"
#include "renderer/texture.h"
#include <array>
#include <optional>
namespace renderer {
float edge(Vec2 a, Vec2 b, Vec2 p);
std::optional<std::array<float, 3>> barycentric(const std::array<Vec2, 3>& triangle, Vec2 p);
struct ScreenVertex {
    Vec3 position;
    float inverse_w = 1;
    Vec2 uv;
    Vec3 normal;
};
struct Surface {
    const Texture* texture = nullptr;
    Color albedo{220, 225, 232};
    Vec3 light{-0.4f, 0.7f, 1};
    bool lit = true;
};
std::array<float, 3> perspective_weights(std::array<float, 3> weights,
                                         const std::array<ScreenVertex, 3>& vertices);
void rasterize(Framebuffer& target, const std::array<ScreenVertex, 3>& triangle,
               const Surface& surface, bool cull = true);
void rasterize(Framebuffer& target, const std::array<Vec3, 3>& triangle, Color color,
               bool cull = true);
} // namespace renderer
