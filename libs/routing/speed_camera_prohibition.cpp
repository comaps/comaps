#include "routing/speed_camera_prohibition.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <vector>

namespace
{
using CountrySetT = std::unordered_set<std::string_view>;

// List of country names where mwm should be generated without speed cameras.
CountrySetT kSpeedCamerasProhibitedCountries = {
    "Germany",
    "Macedonia",
    "Switzerland",
    "Turkey",
    "Bosnia and Herzegovina",
};

// List of country names where an end user should be warned about speed cameras.
CountrySetT kSpeedCamerasPartlyProhibitedCountries = {
    "France",
};

bool IsMwmContained(platform::CountryFile const & mwm, CountrySetT const & countryList)
{
  return std::any_of(countryList.cbegin(), countryList.cend(),
                     [&mwm](auto const & country) { return mwm.GetName().starts_with(country); });
}
}  // namespace

namespace routing
{
bool AreSpeedCamerasProhibited(platform::CountryFile const & mwm)
{
  return IsMwmContained(mwm, kSpeedCamerasProhibitedCountries);
}

bool AreSpeedCamerasPartlyProhibited(platform::CountryFile const & mwm)
{
  return IsMwmContained(mwm, kSpeedCamerasPartlyProhibitedCountries);
}
}  // namespace routing
