#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace veometri::sculpt { class SculptMesh; }

namespace veometri::io {

struct GeometryVertex
{
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 texCoord{};
};

struct GeometryData
{
    std::vector<GeometryVertex> vertices;
    std::vector<std::uint32_t> indices;
};

// Generates area-weighted smooth normals and deterministic planar fallback UVs.
// Hard edges require duplicated vertices in the source indexed mesh.
GeometryData buildGeometryData(const sculpt::SculptMesh& mesh);

} // namespace veometri::io
