#pragma once

#include <string_view>

namespace platform
{
class CountryFile;
}  // namespace platform

namespace routing
{
std::string_view constexpr kDebugSpeedCamSetting = "DebugSpeedCam";

/// \returns true if any information about speed cameras is prohibited in |mwm|.
bool AreSpeedCamerasProhibited(platform::CountryFile const & mwm);

/// \returns true if any information about speed cameras is prohibited or partly prohibited in |mwm|.
bool AreSpeedCamerasPartlyProhibited(platform::CountryFile const & mwm);
}  // namespace routing
