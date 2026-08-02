#include "veometri/io/GeometryFileFormat.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include <glm/geometric.hpp>
#include <nlohmann/json.hpp>

#include "veometri/sculpt/SculptMesh.h"

namespace veometri::io {
namespace {
using Json = nlohmann::ordered_json;
constexpr std::size_t maximumVertices = 5'000'000;
constexpr std::size_t maximumIndices = 15'000'000;

std::string requireField(const Json& value, const char* name, const std::string& context = {})
{
    return value.contains(name) ? std::string{} : context + "Missing required field \"" + name + "\".";
}

bool readVector(const Json& value, std::size_t count, float* output, const std::string& label,
                std::string& error)
{
    if (!value.is_array() || value.size() != count)
    {
        error = label + " must contain exactly " + std::to_string(count) + " numeric components.";
        return false;
    }
    for (std::size_t component = 0; component < count; ++component)
    {
        if (!value[component].is_number())
        {
            error = label + " component " + std::to_string(component) + " must be numeric.";
            return false;
        }
        double number;
        try { number = value[component].get<double>(); }
        catch (const Json::exception&) { error = label + " component " + std::to_string(component) + " is out of range."; return false; }
        if (!std::isfinite(number) || number > std::numeric_limits<float>::max() ||
            number < -std::numeric_limits<float>::max())
        {
            error = label + " component " + std::to_string(component) + " is non-finite or outside the float range.";
            return false;
        }
        output[component] = static_cast<float>(number);
    }
    return true;
}

GeometryFileFormat::DecodeResult failure(std::string error) { return {false, {}, std::move(error)}; }
}

std::string GeometryFileFormat::encode(const GeometryData& geometry)
{
    if (geometry.vertices.size() > maximumVertices || geometry.indices.size() > maximumIndices)
        throw std::invalid_argument("Cannot encode geometry that exceeds count limits.");
    if (geometry.indices.size() % 3 != 0)
        throw std::invalid_argument("Cannot encode geometry whose index count is not divisible by three.");
    Json root = Json::object();
    root["format"] = "veometri-geometry";
    root["version"] = 2;
    root["primitive"] = "triangles";
    root["vertices"] = Json::array();
    for (std::size_t i = 0; i < geometry.vertices.size(); ++i)
    {
        const auto& v = geometry.vertices[i];
        const float values[] = {v.position.x, v.position.y, v.position.z, v.normal.x,
                                v.normal.y, v.normal.z, v.texCoord.x, v.texCoord.y};
        for (float value : values)
            if (!std::isfinite(value)) throw std::invalid_argument("Cannot encode non-finite component at vertex " + std::to_string(i) + ".");
        if (glm::dot(v.normal, v.normal) <= 1.0e-20F)
            throw std::invalid_argument("Cannot encode zero normal at vertex " + std::to_string(i) + ".");
        root["vertices"].push_back({{"position", {v.position.x, v.position.y, v.position.z}},
                                     {"normal", {v.normal.x, v.normal.y, v.normal.z}},
                                     {"texCoord", {v.texCoord.x, v.texCoord.y}}});
    }
    for (std::size_t i = 0; i < geometry.indices.size(); ++i)
        if (geometry.indices[i] >= geometry.vertices.size())
            throw std::invalid_argument("Index " + std::to_string(i) + " references a missing vertex.");
    root["indices"] = geometry.indices;
    return root.dump(2) + '\n';
}

GeometryFileFormat::DecodeResult GeometryFileFormat::decode(std::string_view text)
{
    Json root;
    try { root = Json::parse(text.begin(), text.end()); }
    catch (const Json::exception& error) { return failure("Malformed or out-of-range JSON: " + std::string(error.what())); }
    if (!root.is_object()) return failure("Expected top-level JSON object.");
    for (const char* field : {"format", "version", "primitive", "vertices", "indices"})
        if (auto error = requireField(root, field); !error.empty()) return failure(error);
    if (!root["format"].is_string()) return failure("Field \"format\" must be a string.");
    if (!root["version"].is_number_integer()) return failure("Field \"version\" must be an integer.");
    int version;
    try { version = root["version"].get<int>(); }
    catch (const Json::exception&) { return failure("Field \"version\" is outside the supported integer range."); }
    const std::string format = root["format"].get<std::string>();
    if (!((version == 1 && format == "indexed-geometry") || (version == 2 && format == "veometri-geometry")))
        return failure("Unsupported geometry format/version combination.");
    if (!root["primitive"].is_string() || root["primitive"] != "triangles")
        return failure("Field \"primitive\" must be exactly \"triangles\".");
    if (!root["vertices"].is_array()) return failure("Field \"vertices\" must be an array.");
    if (!root["indices"].is_array()) return failure("Field \"indices\" must be an array.");
    if (root["vertices"].size() > maximumVertices) return failure("Vertex count exceeds the limit of 5000000.");
    if (root["indices"].size() > maximumIndices) return failure("Index count exceeds the limit of 15000000.");

    GeometryData geometry;
    geometry.vertices.resize(root["vertices"].size());
    for (std::size_t i = 0; i < geometry.vertices.size(); ++i)
    {
        const auto& item = root["vertices"][i];
        float position[3], normal[3], uv[2];
        std::string error;
        if (version == 1)
        {
            if (!readVector(item, 3, position, "Vertex " + std::to_string(i), error)) return failure(error);
        }
        else
        {
            if (!item.is_object()) return failure("Vertex " + std::to_string(i) + " must be an object.");
            for (const char* field : {"position", "normal", "texCoord"})
                if (!item.contains(field)) return failure("Vertex " + std::to_string(i) + " is missing required field \"" + field + "\".");
            if (!readVector(item["position"], 3, position, "Vertex " + std::to_string(i) + " position", error) ||
                !readVector(item["normal"], 3, normal, "Vertex " + std::to_string(i) + " normal", error) ||
                !readVector(item["texCoord"], 2, uv, "Vertex " + std::to_string(i) + " texCoord", error)) return failure(error);
            const glm::vec3 n(normal[0], normal[1], normal[2]);
            if (glm::dot(n, n) <= 1.0e-20F) return failure("Vertex " + std::to_string(i) + " normal must be nonzero.");
            geometry.vertices[i].normal = n;
            geometry.vertices[i].texCoord = {uv[0], uv[1]};
        }
        geometry.vertices[i].position = {position[0], position[1], position[2]};
    }
    if (root["indices"].size() % 3 != 0) return failure("Triangle index count must be divisible by three.");
    geometry.indices.reserve(root["indices"].size());
    for (std::size_t i = 0; i < root["indices"].size(); ++i)
    {
        const auto& item = root["indices"][i];
        if (!item.is_number_unsigned()) return failure("Index " + std::to_string(i) + " is not an unsigned integer.");
        const auto value = item.get<std::uint64_t>();
        if (value > std::numeric_limits<std::uint32_t>::max()) return failure("Index " + std::to_string(i) + " exceeds uint32_t.");
        if (value >= geometry.vertices.size()) return failure("Index " + std::to_string(i) + " references missing vertex " + std::to_string(value) + ".");
        geometry.indices.push_back(static_cast<std::uint32_t>(value));
    }
    if (version == 1)
    {
        std::vector<glm::vec3> positions;
        positions.reserve(geometry.vertices.size());
        for (const auto& vertex : geometry.vertices) positions.push_back(vertex.position);
        auto mesh = sculpt::SculptMesh::create(std::move(positions), geometry.indices);
        if (!mesh.success) return failure(mesh.error);
        geometry = buildGeometryData(mesh.mesh);
    }
    return {true, std::move(geometry), {}};
}

} // namespace veometri::io
