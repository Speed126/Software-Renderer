// SPDX-License-Identifier: MIT
#include "renderer/tga_writer.h"
#include "renderer/framebuffer.h"
#include <array>
#include <fstream>
#include <span>
#include <stdexcept>
#include <vector>

namespace renderer {
void write_tga(const Framebuffer& framebuffer, const std::string& path) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    const auto check_output = [&] {
        if (!output)
            throw std::runtime_error("cannot write output: " + path);
    };
    check_output();
    const auto write = [&](std::span<const unsigned char> bytes) {
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        check_output();
    };

    // Encode fields explicitly, independent of host endianness or struct padding.
    std::array<unsigned char, 18> header{};
    header[2] = 2; // Uncompressed true-color image, no color map.
    header[12] = static_cast<unsigned char>(framebuffer.width() & 0xff);
    header[13] = static_cast<unsigned char>(framebuffer.width() >> 8);
    header[14] = static_cast<unsigned char>(framebuffer.height() & 0xff);
    header[15] = static_cast<unsigned char>(framebuffer.height() >> 8);
    header[16] = 24;
    header[17] = 0x20; // Top-left origin, no alpha bits.
    write(header);

    std::vector<unsigned char> row(static_cast<std::size_t>(framebuffer.width()) * 3);
    for (int y = framebuffer.height() - 1; y >= 0; --y) {
        for (int x = 0; x < framebuffer.width(); ++x) {
            const Color color = framebuffer.color(x, y);
            const auto offset = static_cast<std::size_t>(x) * 3;
            row[offset] = color.b;
            row[offset + 1] = color.g;
            row[offset + 2] = color.r;
        }
        write(row);
    }

    // TGA 2.0 footer: no extension/developer sections, then the format signature.
    constexpr std::array<unsigned char, 26> footer{0,   0,   0,   0,   0,   0,   0,   0,   'T',
                                                   'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O', 'N',
                                                   '-', 'X', 'F', 'I', 'L', 'E', '.', 0};
    write(footer);
    output.close();
    check_output();
}
} // namespace renderer
