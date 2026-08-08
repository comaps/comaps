#pragma once

#include "platform/duration.hpp"

#include <string>

namespace platform
{
struct Locale;

std::string GetUnitsString(Duration::Units unit);

std::string GetLocalizedString(Duration const & duration, std::initializer_list<Duration::Units> units,
                               Locale const & locale);
std::string GetPlatformLocalizedString(Duration const & duration);
}  // namespace platform
