// SPDX-License-Identifier: MIT
#pragma once
#include "renderer/framebuffer.h"
#include "renderer/math.h"
#include <filesystem>
#include <istream>
namespace renderer {
class Texture {
  public:
    static Texture checker();
    static Texture load(const std::filesystem::path& path);
    static Texture parse(std::istream& input);
    Color sample(Vec2 uv) const;

  private:
    int width_{}, height_{};
    std::vector<Color> pixels_;
};
Color lambert(Color albedo, Vec3 normal, Vec3 light);
} // namespace renderer
