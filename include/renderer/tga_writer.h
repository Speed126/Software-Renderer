// SPDX-License-Identifier: MIT
#pragma once
#include <string>

namespace renderer {
class Framebuffer;

// Write an opaque, uncompressed 24-bit TGA; throws on output failure.
void write_tga(const Framebuffer& framebuffer, const std::string& path);
} // namespace renderer
