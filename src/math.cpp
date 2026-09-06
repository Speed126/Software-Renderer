// SPDX-License-Identifier: MIT
#include "renderer/math.h"
#include <stdexcept>
namespace renderer {
Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up) {
    const Vec3 forward = normalized(target - eye);
    const Vec3 right = normalized(cross(forward, up));
    const Vec3 vertical = cross(right, forward);
    if (!finite(eye) || !finite(target) || !finite(up) || length(right) < 0.5f)
        throw std::invalid_argument("camera eye/target/up do not define a view");
    Mat4 m = Mat4::identity();
    m.m[0][0] = right.x;
    m.m[0][1] = right.y;
    m.m[0][2] = right.z;
    m.m[0][3] = -dot(right, eye);
    m.m[1][0] = vertical.x;
    m.m[1][1] = vertical.y;
    m.m[1][2] = vertical.z;
    m.m[1][3] = -dot(vertical, eye);
    m.m[2][0] = -forward.x;
    m.m[2][1] = -forward.y;
    m.m[2][2] = -forward.z;
    m.m[2][3] = dot(forward, eye);
    return m;
}
Mat4 perspective(float fov, float aspect, float near_plane, float far_plane) {
    if (!(fov > 0 && fov < 3.14159f && aspect > 0 && near_plane > 0 && far_plane > near_plane) ||
        !std::isfinite(aspect) || !std::isfinite(far_plane))
        throw std::invalid_argument("invalid perspective parameters");
    const float f = 1 / std::tan(fov / 2);
    Mat4 m;
    m.m[0][0] = f / aspect;
    m.m[1][1] = f;
    m.m[2][2] = -(far_plane + near_plane) / (far_plane - near_plane);
    m.m[2][3] = -2 * far_plane * near_plane / (far_plane - near_plane);
    m.m[3][2] = -1;
    return m;
}
Vec3 viewport(Vec4 clip, int width, int height) {
    if (!(clip.w > 0))
        throw std::invalid_argument("perspective divide requires positive w");
    return {(clip.x / clip.w + 1) * static_cast<float>(width) / 2,
            (clip.y / clip.w + 1) * static_cast<float>(height) / 2, (clip.z / clip.w + 1) / 2};
}
} // namespace renderer
