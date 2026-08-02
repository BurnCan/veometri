#include "veometri/io/GeometryFileService.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

using veometri::io::GeometryFileService;
using veometri::sculpt::SculptMesh;
namespace { int failures; void check(bool value, const char* message) { if (!value) { ++failures; std::cerr << "FAIL: " << message << '\n'; } } }

int main()
{
    const auto root = std::filesystem::temp_directory_path() /
        ("geo-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    const auto file = root / "cube.geo";
    const auto cube = SculptMesh::makeDefaultCube();
    check(GeometryFileService::save(file, cube).success && std::filesystem::is_regular_file(file), "save creates file");
    auto loaded = GeometryFileService::load(file);
    check(loaded.success && loaded.mesh.equals(cube), "save/load preserves geometry");
    check(!std::filesystem::exists(file.string() + ".tmp"), "successful save leaves no temporary file");
    check(!GeometryFileService::load(root / "missing.geo").success, "missing file fails clearly");
    check(!GeometryFileService::load(root).success, "directory load fails clearly");
    { std::ofstream bad(root / "bad.geo"); bad << "{"; }
    loaded = GeometryFileService::load(root / "bad.geo");
    check(!loaded.success && loaded.mesh.empty(), "malformed load has no replacement geometry");
    const auto oversized = root / "large.geo";
    { std::ofstream out(oversized, std::ios::binary); out.seekp(static_cast<std::streamoff>(GeometryFileService::maximumFileSize)); out.put('x'); }
    check(!GeometryFileService::load(oversized).success, "oversized file rejected");
    check(!GeometryFileService::save(root / "absent" / "x.geo", cube).success, "invalid parent fails");
    check(!std::filesystem::exists(root / "absent" / "x.geo.tmp"), "failed save leaves no temporary file");
    check(GeometryFileService::save(file, cube).success && GeometryFileService::load(file).success, "existing valid destination replaced");
    const auto unicode = root / std::filesystem::path("géométrie.geo");
    check(GeometryFileService::save(unicode, cube).success && GeometryFileService::load(unicode).success, "Unicode path works");
    check(GeometryFileService::save(root / "extensionless", cube).success && std::filesystem::exists(root / "extensionless.geo"), "extensionless save appends .geo");
    check(!GeometryFileService::save(root / "wrong.meshgeo", cube).success, "incompatible save extension rejected");
    std::filesystem::remove_all(root);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
