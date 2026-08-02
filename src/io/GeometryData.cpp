#include "veometri/io/GeometryData.h"

#include "veometri/sculpt/SculptMesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

#include <glm/geometric.hpp>

namespace veometri::io {

GeometryData buildGeometryData(const sculpt::SculptMesh& mesh)
{
    if (!mesh.isValid())
        throw std::invalid_argument("Cannot export invalid geometry: " + mesh.validationError());

    GeometryData result;
    result.indices = mesh.indices();
    result.vertices.resize(mesh.vertexCount());
    std::vector<glm::vec3> accumulated(mesh.vertexCount(), glm::vec3(0.0F));
    for (std::size_t i = 0; i < mesh.vertexCount(); ++i)
    {
        const auto& p = mesh.vertices()[i];
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
            throw std::invalid_argument("Cannot export non-finite position at vertex " + std::to_string(i) + ".");
        result.vertices[i].position = p;
    }
    for (std::size_t i = 0; i < result.indices.size(); i += 3)
    {
        const auto a = result.indices[i], b = result.indices[i + 1], c = result.indices[i + 2];
        const glm::vec3 face = glm::cross(mesh.vertices()[b] - mesh.vertices()[a],
                                          mesh.vertices()[c] - mesh.vertices()[a]);
        const float squaredLength = glm::dot(face, face);
        if (std::isfinite(squaredLength) && squaredLength > 1.0e-20F)
        {
            accumulated[a] += face;
            accumulated[b] += face;
            accumulated[c] += face;
        }
    }
    for (std::size_t i = 0; i < result.vertices.size(); ++i)
    {
        const float squaredLength = glm::dot(accumulated[i], accumulated[i]);
        result.vertices[i].normal = std::isfinite(squaredLength) && squaredLength > 1.0e-20F
            ? accumulated[i] / std::sqrt(squaredLength) : glm::vec3(0.0F, 1.0F, 0.0F);
    }

    if (result.vertices.empty()) return result;
    glm::vec3 minimum(std::numeric_limits<float>::max());
    glm::vec3 maximum(-std::numeric_limits<float>::max());
    for (const auto& vertex : result.vertices)
    {
        minimum = glm::min(minimum, vertex.position);
        maximum = glm::max(maximum, vertex.position);
    }
    const glm::vec3 extent = maximum - minimum;
    std::array<unsigned int, 3> axes{0, 1, 2};
    std::stable_sort(axes.begin(), axes.end(), [&](unsigned int a, unsigned int b) {
        return extent[a] > extent[b];
    });
    for (auto& vertex : result.vertices)
    {
        const auto project = [&](unsigned int axis) {
            return extent[axis] > 1.0e-10F ? (vertex.position[axis] - minimum[axis]) / extent[axis] : 0.0F;
        };
        vertex.texCoord = {project(axes[0]), project(axes[1])};
    }
    return result;
}

} // namespace veometri::io
