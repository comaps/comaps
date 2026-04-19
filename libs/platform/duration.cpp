#include "duration.hpp"

#include "base/assert.hpp"

/// @todo(KK): move the formatting code from the platform namespace
namespace platform
{
Duration::Duration(unsigned long seconds) : m_seconds(seconds) {}

std::string DebugPrint(Duration::Units units)
{
  switch (units)
  {
  case Duration::Units::Days: return "d";
  case Duration::Units::Hours: return "h";
  case Duration::Units::Minutes: return "m";
  default: UNREACHABLE();
  }
}
}  // namespace platform
