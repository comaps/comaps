#pragma once

#include <string>
#include <vector>

#include "indexer/ftraits.hpp"
#include "indexer/yes_no_unknown.hpp"

struct FeatureID;
class StringUtf8Multilang;

namespace feature
{
static constexpr uint8_t kMaxStarsCount = 7;
static constexpr std::string_view kFieldsSeparator = " • ";
static constexpr std::string_view kToiletsSymbol = "🚻";
static constexpr std::string_view kAtmSymbol = "💳";
static constexpr std::string_view kWheelchairSymbol = "♿️";
static constexpr std::string_view kWifiSymbol = "🛜";

/// OSM internet_access tag values.
enum class Internet
{
  Unknown,   //!< Internet state is unknown (default).
  Wlan,      //!< Wireless Internet access is present.
  Terminal,  //!< A computer with internet service.
  Wired,     //!< Wired Internet access is present.
  Yes,       //!< Unspecified Internet access is available.
  No         //!< There is definitely no any Internet access.
};
std::string DebugPrint(Internet internet);
/// @param[in]  inet  Should be lowercase like in DebugPrint.
Internet InternetFromString(std::string_view inet);

YesNoUnknown YesNoUnknownFromString(std::string_view str);

// Address house numbers interpolation.
enum class InterpolType : uint8_t
{
  None,
  Odd,
  Even,
  Any
};

class TypesHolder;
class RegionData;

/// Get viewport scale to show given feature. Used in search.
int GetFeatureViewportScale(TypesHolder const & types);

/// Format house numbers etc to be more human-readable instead of using symbols like 123:456
std::string const GetReadableAddress(std::string const & address);

/// Returns language id as return result and name for search on booking in the @name parameter,
///  the priority is the following:
/// - default name;
/// - country language name;
/// - english name.
// int8_t GetNameForSearchOnBooking(RegionData const & regionData, StringUtf8Multilang const & src, std::string & name);

// Returns vector of subtypes localized by platform.
std::vector<std::string> GetLocalizedSubtypes(TypesHolder const & types);

// Returns vector of cuisines readable names from classificator.
std::vector<std::string> GetCuisines(TypesHolder const & types);

// Returns vector of cuisines names localized by platform.
std::vector<std::string> GetLocalizedCuisines(TypesHolder const & types);

// Returns fee type localized by platform.
std::string GetLocalizedFeeType(TypesHolder const & types);

// Returns readable wheelchair type.
std::string GetReadableWheelchairType(TypesHolder const & types);

/// @returns wheelchair availability.
std::optional<ftraits::WheelchairAvailability> GetWheelchairType(TypesHolder const & types);

/// Returns true if feature has ATM type.
bool HasAtm(TypesHolder const & types);

/// Returns true if feature has Toilets type.
bool HasToilets(TypesHolder const & types);

/// @returns formatted drinking water type.
std::string FormatDrinkingWater(TypesHolder const & types);

/// @returns starsCount of ★ symbol.
std::string FormatStars(uint8_t starsCount);

/// @returns formatted elevation with ▲ symbol and units.
std::string FormatElevation(std::string_view elevation);

}  // namespace feature
