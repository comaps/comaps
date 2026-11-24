#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace coding
{
class SHA256
{
public:
  static size_t constexpr kHashSizeInBytes = 32;
  using Hash = std::array<uint8_t, kHashSizeInBytes>;

  static Hash CalculateForString(std::string_view str);
};
}  // namespace coding
