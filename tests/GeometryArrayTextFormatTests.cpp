#include "veometri/io/GeometryArrayTextFormat.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <locale>

using veometri::io::GeometryArrayTextFormat;
using veometri::io::GeometryData;
using veometri::io::GeometryVertex;
namespace {
int failures;
void check(bool value, const char *message) {
    if (!value) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}
} // namespace

int main() {
    GeometryData triangle{{{{0.0F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.5F, 1.0F}},
                           {{-0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
                           {{0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}}},
                          {0, 1, 2}};
    const std::string golden = "// positions          // normals           // texcoords\n"
                               "{\n"
                               "    0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f,\n"
                               "    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,\n"
                               "    0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f\n"
                               "};\n\n"
                               "{\n"
                               "    0, 1, 2\n"
                               "};\n";
    check(GeometryArrayTextFormat::encode(triangle) == golden,
          "triangle has deterministic interleaved rows and index array");

    triangle.indices = {2, 1, 0, 0, 2, 1};
    const auto multiple = GeometryArrayTextFormat::encode(triangle);
    check(multiple.find("    2, 1, 0,\n    0, 2, 1\n") != std::string::npos,
          "multiple triangles retain index order and line grouping");

    triangle.vertices[0].position.x = -0.0F;
    check(GeometryArrayTextFormat::encode(triangle).find("-0.0f") == std::string::npos,
          "negative zero is normalized");

    struct CommaPunctuation : std::numpunct<char> {
        char do_decimal_point() const override { return ','; }
    };
    const auto previousLocale = std::locale();
    std::locale::global(std::locale(previousLocale, new CommaPunctuation));
    const auto localeText = GeometryArrayTextFormat::encode(triangle);
    std::locale::global(previousLocale);
    check(localeText.find("0.5f") != std::string::npos, "floats are locale independent");

    triangle.vertices[0].normal.x = std::numeric_limits<float>::infinity();
    try {
        (void)GeometryArrayTextFormat::encode(triangle);
        check(false, "non-finite values are rejected");
    } catch (const std::invalid_argument &) {
    }
    triangle.vertices[0].normal.x = 0.0F;
    triangle.indices = {0, 1, 3};
    try {
        (void)GeometryArrayTextFormat::encode(triangle);
        check(false, "out-of-range indices are rejected");
    } catch (const std::invalid_argument &) {
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
