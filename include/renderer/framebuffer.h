// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace renderer {
struct Color {
    std::uint8_t r{}, g{}, b{};
    bool operator==(const Color&) const = default;
};
class Framebuffer {
  public:
    Framebuffer(int width, int height);
    int width() const { return width_; }
    int height() const { return height_; }
    bool contains(int x, int y) const;
    bool write(int x, int y, float depth, Color color);
    Color color(int x, int y) const;
    float depth(int x, int y) const;
    void save(const std::string& path) const;

  private:
    int width_, height_;
    std::vector<Color> colors_;
    std::vector<float> depths_;
    std::size_t index(int x, int y) const;
};
} // namespace renderer
