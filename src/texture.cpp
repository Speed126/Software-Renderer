// SPDX-License-Identifier: MIT
#include "renderer/texture.h"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <stdexcept>
namespace renderer {
namespace {
std::string token(std::istream& input) {
    std::string word;
    while (input >> word) {
        if (word.starts_with('#')) {
            std::getline(input, word);
            continue;
        }
        return word;
    }
    throw std::runtime_error("truncated PPM texture");
}
int integer(std::istream& input, int low, int high) {
    const std::string word = token(input);
    int value{};
    const auto [end, error] = std::from_chars(word.data(), word.data() + word.size(), value);
    if (error != std::errc{} || end != word.data() + word.size() || value < low || value > high)
        throw std::runtime_error("invalid PPM integer: " + word);
    return value;
}
} // namespace
Texture Texture::checker() {
    Texture result;
    result.width_ = 96;
    result.height_ = 48;
    for (int y = 0; y < result.height_; ++y)
        for (int x = 0; x < result.width_; ++x)
            result.pixels_.push_back(((x / 8 + y / 8) % 2) == 0 ? Color{231, 197, 132}
                                                                : Color{34, 150, 162});
    return result;
}
Texture Texture::load(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("cannot open texture: " + path.string());
    try {
        return parse(input);
    } catch (const std::exception& e) {
        throw std::runtime_error(path.string() + ": " + e.what());
    }
}
Texture Texture::parse(std::istream& input) {
    if (token(input) != "P3")
        throw std::runtime_error("texture must be an ASCII P3 PPM image");
    Texture result;
    result.width_ = integer(input, 1, 4096);
    result.height_ = integer(input, 1, 4096);
    const int maximum = integer(input, 1, 65535);
    const auto count =
        static_cast<std::size_t>(result.width_) * static_cast<std::size_t>(result.height_);
    result.pixels_.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto r = static_cast<std::uint8_t>(integer(input, 0, maximum) * 255 / maximum);
        const auto g = static_cast<std::uint8_t>(integer(input, 0, maximum) * 255 / maximum);
        const auto b = static_cast<std::uint8_t>(integer(input, 0, maximum) * 255 / maximum);
        result.pixels_.push_back({r, g, b});
    }
    return result;
}
Color Texture::sample(Vec2 uv) const {
    if (pixels_.empty() || !std::isfinite(uv.x) || !std::isfinite(uv.y))
        return {255, 0, 255};
    const float u = uv.x - std::floor(uv.x), v = uv.y - std::floor(uv.y);
    const int x = std::min(width_ - 1, static_cast<int>(u * static_cast<float>(width_)));
    const int y =
        height_ - 1 - std::min(height_ - 1, static_cast<int>(v * static_cast<float>(height_)));
    return pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                   static_cast<std::size_t>(x)];
}
Color lambert(Color albedo, Vec3 normal, Vec3 light) {
    const float intensity =
        0.14f + 0.86f * std::max(0.0f, dot(normalized(normal), normalized(light)));
    const auto channel = [intensity](std::uint8_t c) {
        return static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(c) * intensity, 0.0f, 255.0f));
    };
    return {channel(albedo.r), channel(albedo.g), channel(albedo.b)};
}
} // namespace renderer
