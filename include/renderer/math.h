// SPDX-License-Identifier: MIT
#pragma once
#include <cmath>

namespace renderer {
struct Vec2 {
    float x{}, y{};
    Vec2 operator+(Vec2 b) const { return {x + b.x, y + b.y}; }
    Vec2 operator-(Vec2 b) const { return {x - b.x, y - b.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
};
struct Vec3 {
    float x{}, y{}, z{};
    Vec3 operator+(Vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(Vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};
inline float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float length(Vec3 a) {
    return std::sqrt(dot(a, a));
}
inline Vec3 normalized(Vec3 a) {
    const float n = length(a);
    return n > 1e-12f ? a * (1.0f / n) : Vec3{};
}
inline bool finite(Vec3 a) {
    return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z);
}
} // namespace renderer

namespace renderer {
struct Vec4 {
    float x{}, y{}, z{}, w{};
    Vec4 operator+(Vec4 b) const { return {x + b.x, y + b.y, z + b.z, w + b.w}; }
    Vec4 operator-(Vec4 b) const { return {x - b.x, y - b.y, z - b.z, w - b.w}; }
    Vec4 operator*(float s) const { return {x * s, y * s, z * s, w * s}; }
};
// Row-major storage, column vectors: projection * view * model * position.
struct Mat4 {
    float m[4][4]{};
    static Mat4 identity() {
        Mat4 result;
        for (int i = 0; i < 4; ++i)
            result.m[i][i] = 1;
        return result;
    }
    Mat4 operator*(const Mat4& b) const {
        Mat4 result;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                for (int k = 0; k < 4; ++k)
                    result.m[r][c] += m[r][k] * b.m[k][c];
        return result;
    }
    Vec4 operator*(Vec4 v) const {
        const float a[4]{v.x, v.y, v.z, v.w};
        float b[4]{};
        for (int r = 0; r < 4; ++r)
            for (int k = 0; k < 4; ++k)
                b[r] += m[r][k] * a[k];
        return {b[0], b[1], b[2], b[3]};
    }
};
inline Mat4 translation(Vec3 t) {
    Mat4 m = Mat4::identity();
    m.m[0][3] = t.x;
    m.m[1][3] = t.y;
    m.m[2][3] = t.z;
    return m;
}
inline Mat4 scaling(Vec3 s) {
    Mat4 m = Mat4::identity();
    m.m[0][0] = s.x;
    m.m[1][1] = s.y;
    m.m[2][2] = s.z;
    return m;
}
inline Mat4 rotation_y(float radians) {
    Mat4 m = Mat4::identity();
    m.m[0][0] = m.m[2][2] = std::cos(radians);
    m.m[0][2] = std::sin(radians);
    m.m[2][0] = -std::sin(radians);
    return m;
}
Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up);
Mat4 perspective(float vertical_fov, float aspect, float near_plane, float far_plane);
Vec3 viewport(Vec4 clip, int width, int height);
} // namespace renderer
