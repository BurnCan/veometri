#pragma once

#include <string>

#include "veometri/io/GeometryData.h"

namespace veometri::io {

// Encodes portable, declaration-free interleaved vertex and index arrays.
class GeometryArrayTextFormat {
  public:
    static std::string encode(const GeometryData &geometry);
};

} // namespace veometri::io
