// SPDX-License-Identifier: MIT
#include "renderer/pipeline.h"
#include <charconv>
#include <iostream>
#include <stdexcept>
#include <string>

int dimension(const std::string& text) {
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value < 1 || value > 4096)
        throw std::runtime_error("dimensions must be integers in [1, 4096]");
    return value;
}
float number(const std::string& text) {
    float value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || !std::isfinite(value) ||
        std::abs(value) > 10000)
        throw std::runtime_error("expected a finite number with magnitude <= 10000: " + text);
    return value;
}
int main(int argc, char** argv) {
    try {
        std::string input = "assets/orbital.obj", output = "output.tga";
        int width = 800, height = 800;
        bool has_model = false;
        renderer::RenderOptions options;
        renderer::Texture texture = renderer::Texture::checker();
        bool textured = true;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--help") {
                std::cout
                    << "Usage: renderer [model.obj] [options]\n"
                       "Default model: assets/orbital.obj; output: output.tga; size: 800x800\n"
                       "  --output path.tga       Output path (parent must exist)\n"
                       "  --width N --height N    Image size, each 1..4096\n"
                       "  --eye X Y Z             Camera in fitted model coordinates\n"
                       "  --rotate degrees        Model rotation around Y\n"
                       "  --light X Y Z           Direction toward the light\n"
                       "  --texture image.ppm     ASCII P3 texture (default: checker)\n"
                       "  --solid                 Neutral solid surface\n"
                       "  --no-cull               Render both triangle windings\n"
                       "  --help                  Show this help\n";
                return 0;
            }
            if (arg == "--solid") {
                textured = false;
                continue;
            }
            if (arg == "--texture") {
                if (++i == argc)
                    throw std::runtime_error("--texture requires a P3 PPM path");
                texture = renderer::Texture::load(argv[i]);
                textured = true;
                continue;
            }
            if (arg == "--eye" || arg == "--light") {
                if (i + 3 >= argc)
                    throw std::runtime_error(arg + " requires x y z");
                const float x = number(argv[++i]), y = number(argv[++i]), z = number(argv[++i]);
                if (arg == "--eye")
                    options.eye = renderer::Vec3{x, y, z};
                else {
                    if (renderer::length({x, y, z}) < 1e-8f)
                        throw std::runtime_error("light direction must be nonzero");
                    options.light = {x, y, z};
                }
                continue;
            }
            if (arg == "--no-cull") {
                options.cull = false;
                continue;
            }
            if (arg == "--rotate") {
                if (++i == argc)
                    throw std::runtime_error("--rotate requires degrees");
                options.rotation = number(argv[i]) * 0.01745329252f;
                continue;
            }
            if (arg == "--output" || arg == "--width" || arg == "--height") {
                if (++i == argc)
                    throw std::runtime_error("missing value for " + arg);
                if (arg == "--output")
                    output = argv[i];
                else if (arg == "--width")
                    width = dimension(argv[i]);
                else
                    height = dimension(argv[i]);
            } else if (arg.starts_with("-") || has_model) {
                throw std::runtime_error("unexpected argument: " + arg);
            } else {
                input = arg;
                has_model = true;
            }
        }
        const auto model = renderer::Model::load(input);
        options.texture = textured ? &texture : nullptr;
        renderer::Framebuffer image(width, height);
        renderer::render(model, image, options);
        image.save(output);
        std::cout << "Processed " << model.faces.size() << " input triangles; wrote " << output
                  << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "renderer: " << error.what() << '\n';
        return 1;
    }
}
