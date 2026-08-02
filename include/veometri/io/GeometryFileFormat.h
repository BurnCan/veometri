#pragma once

#include <string>
#include <string_view>

#include "veometri/io/GeometryData.h"

namespace veometri::io {

class GeometryFileFormat
{
public:
    struct DecodeResult
    {
        bool success = false;
        GeometryData geometry;
        std::string error;
    };

    static std::string encode(const GeometryData& geometry);
    static DecodeResult decode(std::string_view text);
};

} // namespace veometri::io
