// SPDX-License-Identifier: MIT
#include "renderer/model.h"
#include <charconv>
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace renderer {
namespace {
int resolve(const std::string& token, std::size_t count) {
    int index{};
    const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), index);
    if (error != std::errc{} || end != token.data() + token.size() || index == 0)
        throw std::runtime_error("invalid OBJ index: " + token);
    const auto resolved =
        index > 0 ? static_cast<long long>(index) - 1 : static_cast<long long>(count) + index;
    if (resolved < 0 || resolved >= static_cast<long long>(count))
        throw std::runtime_error("OBJ index out of range: " + token);
    return static_cast<int>(resolved);
}
Corner corner(const std::string& token, const Model& model) {
    const auto first = token.find('/');
    Corner c;
    c.position = static_cast<std::size_t>(resolve(token.substr(0, first), model.positions.size()));
    if (first == std::string::npos)
        return c;
    const auto second = token.find('/', first + 1);
    if (second == std::string::npos) {
        c.uv = resolve(token.substr(first + 1), model.uvs.size());
    } else {
        if (token.find('/', second + 1) != std::string::npos)
            throw std::runtime_error("too many index fields");
        if (second > first + 1)
            c.uv = resolve(token.substr(first + 1, second - first - 1), model.uvs.size());
        c.normal = resolve(token.substr(second + 1), model.normals.size());
    }
    return c;
}
} // namespace
Model Model::load(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("cannot open model: " + path.string());
    return parse(input, path.string());
}
Model Model::parse(std::istream& input, const std::string& source) {
    Model model;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        line = line.substr(0, line.find('#'));
        std::istringstream fields(line);
        std::string type;
        if (!(fields >> type))
            continue;
        try {
            if (type == "v" || type == "vn") {
                Vec3 v;
                if (!(fields >> v.x >> v.y >> v.z) || !finite(v) || std::abs(v.x) > 1e10f ||
                    std::abs(v.y) > 1e10f || std::abs(v.z) > 1e10f)
                    throw std::runtime_error(
                        "expected three finite coordinates (magnitude <= 1e10)");
                std::string extra;
                if (fields >> extra)
                    throw std::runtime_error("extra coordinate fields are unsupported");
                if (type == "v")
                    model.positions.push_back(v);
                else {
                    if (length(v) < 1e-12f)
                        throw std::runtime_error("normal must be nonzero");
                    model.normals.push_back(normalized(v));
                }
            } else if (type == "vt") {
                Vec2 uv;
                if (!(fields >> uv.x >> uv.y) || !std::isfinite(uv.x) || !std::isfinite(uv.y))
                    throw std::runtime_error("expected two finite UV coordinates");
                std::string extra;
                if (fields >> extra) {
                    float third{};
                    const auto [end, error] =
                        std::from_chars(extra.data(), extra.data() + extra.size(), third);
                    if (error != std::errc{} || end != extra.data() + extra.size() ||
                        !std::isfinite(third))
                        throw std::runtime_error("invalid optional third texture coordinate");
                    if (fields >> extra)
                        throw std::runtime_error("too many texture coordinates");
                    // OBJ permits a third coordinate; this renderer samples 2D textures.
                }
                model.uvs.push_back(uv);
            } else if (type == "f") {
                Face face;
                std::string token;
                for (auto& c : face) {
                    if (!(fields >> token))
                        throw std::runtime_error("face needs three corners");
                    c = corner(token, model);
                }
                if (fields >> token)
                    throw std::runtime_error(
                        "polygon has more than three corners; triangulate the OBJ before loading");
                model.faces.push_back(face);
            }
            // Object/group/smoothing/material metadata is deliberately ignored.
        } catch (const std::exception& error) {
            throw std::runtime_error(source + ":" + std::to_string(line_number) + ": " +
                                     error.what());
        }
    }
    if (input.bad())
        throw std::runtime_error(source + ": read failed");
    if (model.faces.empty())
        throw std::runtime_error(source + ": model contains no triangles");
    return model;
}
} // namespace renderer
