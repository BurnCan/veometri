#include "veometri/io/GeometryArrayTextFormat.h"

#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace veometri::io {
namespace {
std::string floatLiteral(float value) {
    if (!std::isfinite(value))
        throw std::invalid_argument("Cannot encode a non-finite vertex value.");
    if (value == 0.0F)
        return "0.0f";

    char buffer[64];
    const auto result =
        std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general,
                      std::numeric_limits<float>::max_digits10);
    if (result.ec != std::errc{})
        throw std::runtime_error("Could not format a vertex value.");
    std::string literal(buffer, result.ptr);
    if (literal.find_first_of(".eE") == std::string::npos)
        literal += ".0";
    return literal + 'f';
}
} // namespace

std::string GeometryArrayTextFormat::encode(const GeometryData &geometry) {
    if (geometry.indices.size() % 3 != 0)
        throw std::invalid_argument(
            "Cannot encode geometry whose index count is not divisible by three.");
    for (std::size_t i = 0; i < geometry.indices.size(); ++i)
        if (geometry.indices[i] >= geometry.vertices.size())
            throw std::invalid_argument("Index " + std::to_string(i) +
                                        " references a missing vertex.");

    std::string output = "{\n // positions          // normals           // texcoords\n";
    for (std::size_t i = 0; i < geometry.vertices.size(); ++i) {
        const auto &vertex = geometry.vertices[i];
        const float values[] = {vertex.position.x, vertex.position.y, vertex.position.z,
                                vertex.normal.x,   vertex.normal.y,   vertex.normal.z,
                                vertex.texCoord.x, vertex.texCoord.y};
        output += "    ";
        for (std::size_t component = 0; component < 8; ++component) {
            output += floatLiteral(values[component]);
            if (component != 7)
                output += ", ";
        }
        if (i + 1 != geometry.vertices.size())
            output += ',';
        output += '\n';
    }
    output += "};\n\n{\n";
    for (std::size_t i = 0; i < geometry.indices.size(); i += 3) {
        output += "    " + std::to_string(geometry.indices[i]) + ", " +
                  std::to_string(geometry.indices[i + 1]) + ", " +
                  std::to_string(geometry.indices[i + 2]);
        if (i + 3 != geometry.indices.size())
            output += ',';
        output += '\n';
    }
    output += "};\n";
    return output;
}

} // namespace veometri::io
