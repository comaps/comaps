#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace indoor
{
// Parses an OSM level=* value into a list of numeric levels.
// Supported forms: "0", "-1", "1.5", "0;1;2", "0-2" (integer ranges), "-2--1" and "," as a separator fallback.
// Returns an empty vector if the value can't be parsed (e.g. "G", "ground").
std::vector<double> ParseLevels(std::string_view s);

// Returns true if the level=* value |s| includes |level|.
// A missing or unparsable value is treated as level 0 (OSM convention for indoor features).
bool LevelsContain(std::string_view s, double level);

// Formats a level for UI display: "0", "-1", "1.5".
std::string FormatLevel(double level);
}  // namespace indoor
