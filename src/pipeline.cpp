// SPDX-License-Identifier: MIT
#include "renderer/pipeline.h"
#include "renderer/rasterizer.h"
#include <algorithm>
#include <stdexcept>
namespace renderer {
std::vector<ClipVertex> clip_triangle(const std::array<ClipVertex, 3>& triangle) {
    std::vector<ClipVertex> polygon(triangle.begin(), triangle.end());
    // Clip homogeneous coordinates BEFORE division, against -w <= x,y,z <= w.
    for (int plane = 0; plane < 6 && !polygon.empty(); ++plane) {
        const auto distance = [plane](Vec4 v) {
            switch (plane) {
            case 0:
                return v.w + v.x;
            case 1:
                return v.w - v.x;
            case 2:
                return v.w + v.y;
            case 3:
                return v.w - v.y;
            case 4:
                return v.w + v.z;
            default:
                return v.w - v.z;
            }
        };
        std::vector<ClipVertex> clipped;
        ClipVertex a = polygon.back();
        float da = distance(a.position);
        for (ClipVertex b : polygon) {
            const float db = distance(b.position);
            if ((da >= 0) != (db >= 0)) {
                const float t = da / (da - db);
                clipped.push_back({a.position + (b.position - a.position) * t,
                                   a.uv + (b.uv - a.uv) * t, a.normal + (b.normal - a.normal) * t});
            }
            if (db >= 0)
                clipped.push_back(b);
            a = b;
            da = db;
        }
        polygon = std::move(clipped);
    }
    return polygon;
}
void render(const Model& model, Framebuffer& target, const RenderOptions& options) {
    if (model.positions.empty() || model.faces.empty())
        throw std::runtime_error("model contains no triangles");
    Vec3 low = model.positions.front(), high = low;
    for (const Vec3 v : model.positions) {
        low = {std::min(low.x, v.x), std::min(low.y, v.y), std::min(low.z, v.z)};
        high = {std::max(high.x, v.x), std::max(high.y, v.y), std::max(high.z, v.z)};
    }
    const Vec3 center = (low + high) * 0.5f, extent = high - low;
    const float size = std::max({extent.x, extent.y, extent.z});
    if (!(size > 1e-8f) || !std::isfinite(size))
        throw std::runtime_error("model has invalid or zero extent");
    const Mat4 model_matrix = rotation_y(options.rotation) *
                              scaling({2 / size, 2 / size, 2 / size}) * translation(center * (-1));
    const float aspect = static_cast<float>(target.width()) / static_cast<float>(target.height());
    const Vec3 eye = options.eye.value_or(Vec3{0.5f, 0.25f, 3.6f * std::max(1.0f, 1 / aspect)});
    const Mat4 view_projection =
        perspective(0.78539816f, aspect, 0.1f, 100.0f) * look_at(eye, {0, 0, 0}, {0, 1, 0});
    const Mat4 normal_rotation = rotation_y(options.rotation);
    for (const auto& face : model.faces) {
        std::array<Vec3, 3> world{};
        std::array<ClipVertex, 3> clip{};
        bool has_uv = true, has_normal = true;
        for (std::size_t j = 0; j < 3; ++j) {
            const auto& corner = face[j];
            const Vec3 v = model.positions.at(corner.position);
            const Vec4 p = model_matrix * Vec4{v.x, v.y, v.z, 1};
            world[j] = {p.x, p.y, p.z};
            clip[j].position = view_projection * p;
            if (corner.uv >= 0)
                clip[j].uv = model.uvs.at(static_cast<std::size_t>(corner.uv));
            else
                has_uv = false;
            if (corner.normal >= 0) {
                const Vec3 n = model.normals.at(static_cast<std::size_t>(corner.normal));
                const Vec4 rotated = normal_rotation * Vec4{n.x, n.y, n.z, 0};
                clip[j].normal = {rotated.x, rotated.y, rotated.z};
            } else
                has_normal = false;
        }
        if (!has_normal) {
            const Vec3 normal = normalized(cross(world[1] - world[0], world[2] - world[0]));
            for (auto& vertex : clip)
                vertex.normal = normal;
        }
        Surface surface;
        surface.texture = has_uv ? options.texture : nullptr;
        surface.light = options.light;
        const auto polygon = clip_triangle(clip);
        const auto screen = [&target](const ClipVertex& vertex) {
            return ScreenVertex{viewport(vertex.position, target.width(), target.height()),
                                1 / vertex.position.w, vertex.uv, vertex.normal};
        };
        for (std::size_t j = 1; j + 1 < polygon.size(); ++j) {
            if (polygon[0].position.w <= 0 || polygon[j].position.w <= 0 ||
                polygon[j + 1].position.w <= 0)
                continue;
            rasterize(target, {screen(polygon[0]), screen(polygon[j]), screen(polygon[j + 1])},
                      surface, options.cull);
        }
    }
}
} // namespace renderer
