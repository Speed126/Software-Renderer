// SPDX-License-Identifier: MIT
#pragma once
#include "renderer/framebuffer.h"
#include "renderer/model.h"
#include "renderer/texture.h"
#include <array>
#include <optional>
#include <vector>
namespace renderer {
struct RenderOptions {
    std::optional<Vec3> eye;
    float rotation{};
    bool cull = true;
    const Texture* texture = nullptr;
    Vec3 light{-0.4f, 0.7f, 1};
};
struct ClipVertex {
    Vec4 position;
    Vec2 uv;
    Vec3 normal;
};
std::vector<ClipVertex> clip_triangle(const std::array<ClipVertex, 3>& triangle);
void render(const Model& model, Framebuffer& target, const RenderOptions& options = {});
} // namespace renderer
