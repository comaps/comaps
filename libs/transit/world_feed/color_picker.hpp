#pragma once

#include "drape/color.hpp"

#include <map>
#include <string>

#include "3party/ankerl/unordered_dense.h"

namespace transit
{
class ColorPicker
{
public:
  ColorPicker();
  // Picks the closest match for the |rgb| color from our transit palette.
  std::string GetNearestColor(std::string const & rgb);

private:
  ankerl::unordered_dense::map<std::string, std::string> m_colorsToNames;
  std::map<std::string, dp::Color> m_drapeClearColors;
};
}  // namespace transit
