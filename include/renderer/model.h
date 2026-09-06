// SPDX-License-Identifier: MIT
#pragma once
#include "renderer/math.h"
#include <array>
#include <filesystem>
#include <istream>
#include <string>
#include <vector>
namespace renderer {
struct Corner {
    std::size_t position{};
    int uv = -1;
    int normal = -1;
};
using Face = std::array<Corner, 3>;
struct Model {
    std::vector<Vec3> positions;
    std::vector<Vec2> uvs;
    std::vector<Vec3> normals;
    std::vector<Face> faces;
    static Model load(const std::filesystem::path& path);
    static Model parse(std::istream& input, const std::string& source = "OBJ");
};
} // namespace renderer
