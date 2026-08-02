#include "veometri/io/GeometryFileFormat.h"
#include "veometri/io/GeometryData.h"
#include "veometri/sculpt/SculptMesh.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>

#include <glm/geometric.hpp>

using namespace veometri;
namespace { int failures; void check(bool v, const char* m) { if (!v) { ++failures; std::cerr << "FAIL: " << m << '\n'; } } }

int main()
{
    const auto cube = sculpt::SculptMesh::makeDefaultCube();
    const auto complete = io::buildGeometryData(cube);
    const auto encoded = io::GeometryFileFormat::encode(complete);
    check(encoded == io::GeometryFileFormat::encode(complete), "encoding deterministic");
    check(encoded.find("\"format\": \"veometri-geometry\"") != std::string::npos &&
          encoded.find("\"version\": 2") != std::string::npos && encoded.find("\"texCoord\"") != std::string::npos,
          "canonical v2 fields encoded");
    auto decoded = io::GeometryFileFormat::decode(encoded);
    check(decoded.success && decoded.geometry.indices == cube.indices() && decoded.geometry.vertices.size() == cube.vertexCount(),
          "v2 round trip preserves topology");
    for (const auto& vertex : complete.vertices)
    {
        check(std::isfinite(vertex.normal.x) && std::abs(glm::length(vertex.normal) - 1.0F) < 1e-5F, "normal finite and normalized");
        check(std::isfinite(vertex.texCoord.x) && vertex.texCoord.x >= 0 && vertex.texCoord.x <= 1 &&
              vertex.texCoord.y >= 0 && vertex.texCoord.y <= 1, "UV finite and normalized");
    }
    auto triangle = sculpt::SculptMesh::create({{0,0,0},{1,0,0},{0,1,0}}, {0,1,2});
    auto generated = io::buildGeometryData(triangle.mesh);
    check(generated.vertices[0].normal.z > 0.99F, "winding controls normal direction");
    auto degenerate = sculpt::SculptMesh::create({{0,0,0},{1,0,0},{2,0,0}}, {0,1,2});
    generated = io::buildGeometryData(degenerate.mesh);
    check(generated.vertices[0].normal == glm::vec3(0,1,0), "degenerate triangle uses deterministic fallback");
    const std::string v1 = R"({"format":"indexed-geometry","version":1,"primitive":"triangles","vertices":[[0,0,0],[1,0,0],[0,1,0]],"indices":[0,1,2]})";
    decoded = io::GeometryFileFormat::decode(v1);
    check(decoded.success && decoded.geometry.vertices[0].normal.z > .99F, "v1 migrates to complete geometry");
    const std::string p = R"({"format":"veometri-geometry","version":2,"primitive":"triangles","vertices":)";
    check(!io::GeometryFileFormat::decode(p + R"([{"position":[0,0,0],"normal":[0,0,1]}],"indices":[0,0,0]})").success, "missing UV rejected");
    check(!io::GeometryFileFormat::decode(p + R"([{"position":[0,0],"normal":[0,0,1],"texCoord":[0,0]}],"indices":[0,0,0]})").success, "position size rejected");
    check(!io::GeometryFileFormat::decode(p + R"([{"position":[0,0,0],"normal":[0,0,0],"texCoord":[0,0]}],"indices":[0,0,0]})").success, "zero normal rejected");
    check(!io::GeometryFileFormat::decode(p + R"([{"position":[0,0,0],"normal":[0,0,1],"texCoord":[0]}],"indices":[0,0,0]})").success, "UV size rejected");
    check(!io::GeometryFileFormat::decode(p + R"([{"position":[0,0,0],"normal":[0,0,1],"texCoord":[0,0]}],"indices":[-1,0,0]})").success, "negative index rejected");
    check(!io::GeometryFileFormat::decode(p + R"([{"position":[0,0,0],"normal":[0,0,1],"texCoord":[0,0]}],"indices":[0.5,0,0]})").success, "floating index rejected");
    check(!io::GeometryFileFormat::decode(p + R"([],"indices":[0]})").success, "non-triangle indices rejected");
    std::ifstream example(VEOMETRI_EXAMPLE_FILE);
    const std::string exampleText((std::istreambuf_iterator<char>(example)), {});
    check(io::GeometryFileFormat::decode(exampleText).success, "checked-in cube.geo loads");
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
