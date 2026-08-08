#pragma once

#include <chrono>
#include <string>

namespace platform
{
struct Locale;

class Duration
{
public:
  enum class Units
  {
    Days = 0,
    Hours = 1,
    Minutes = 2,
  };

  explicit Duration(unsigned long seconds);

  std::chrono::seconds const & GetUnderlying() const noexcept { return m_seconds; }

private:
  std::chrono::seconds const m_seconds;
};

std::string DebugPrint(Duration::Units units);

}  // namespace platform
