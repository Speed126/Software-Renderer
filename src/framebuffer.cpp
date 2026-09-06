// SPDX-License-Identifier: MIT
#include "renderer/framebuffer.h"
#include "renderer/tga_writer.h"
#include <cmath>
#include <stdexcept>
namespace renderer {
Framebuffer::Framebuffer(int width, int height) : width_(width), height_(height) {
    if (width < 1 || height < 1 || width > 4096 || height > 4096)
        throw std::invalid_argument("framebuffer dimensions must be in [1, 4096]");
    const auto count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    colors_.assign(count, {18, 24, 34});
    depths_.assign(count, std::numeric_limits<float>::infinity());
}
bool Framebuffer::contains(int x, int y) const {
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}
std::size_t Framebuffer::index(int x, int y) const {
    if (!contains(x, y))
        throw std::out_of_range("framebuffer coordinate out of range");
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(x);
}
bool Framebuffer::write(int x, int y, float z, Color c) {
    if (!contains(x, y) || !std::isfinite(z) || z < 0 || z > 1)
        return false;
    const auto i = index(x, y);
    if (z >= depths_[i])
        return false;
    depths_[i] = z;
    colors_[i] = c;
    return true;
}
Color Framebuffer::color(int x, int y) const {
    return colors_[index(x, y)];
}
float Framebuffer::depth(int x, int y) const {
    return depths_[index(x, y)];
}
void Framebuffer::save(const std::string& path) const {
    write_tga(*this, path);
}
} // namespace renderer
