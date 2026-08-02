#include "veometri/platform/AssetLocator.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

int main()
{
    const auto prefix = std::filesystem::temp_directory_path() /
        "veometri_asset_locator_test" / "prefix";
    const auto executable = prefix / "bin" / "veometri";
    const auto roots = veometri::platform::AssetLocator::candidateRoots(executable);
    const auto adjacent = (prefix / "bin" / "assets").lexically_normal();
    const auto installed = (prefix / "share" / "veometri").lexically_normal();
    const auto development =
        std::filesystem::path(VEOMETRI_TEST_DEVELOPMENT_ASSET_ROOT).lexically_normal();
    if (roots.size() != 3 || roots[0].lexically_normal() != adjacent ||
        roots[1].lexically_normal() != installed || roots[2].lexically_normal() != development)
    {
        std::cerr << "Asset candidate order or installed layout is incorrect\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
