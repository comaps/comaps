#include "indexer/feature_utils.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/ftypes_subtypes.hpp"
#include "indexer/scales.hpp"

#include "platform/distance.hpp"
#include "platform/localization.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "coding/string_utf8_multilang.hpp"
#include "coding/transliteration.hpp"

#include "base/localisation_translation.hpp"
#include "base/control_flow.hpp"

#include <utility>

#include "3party/ankerl/unordered_dense.h"

namespace feature
{
using namespace std;

namespace
{

// Filters types with |checker|, returns vector of raw type second components.
// For example for types {"cuisine-sushi", "cuisine-pizza", "cuisine-seafood"} vector
// of second components is {"sushi", "pizza", "seafood"}.
vector<string> GetRawTypeSecond(ftypes::BaseChecker const & checker, TypesHolder const & types)
{
  auto const & c = classif();
  vector<string> res;
  for (auto const t : types)
  {
    if (!checker(t))
      continue;
    auto path = c.GetFullObjectNamePath(t);
    CHECK_EQUAL(path.size(), 2, (path));
    res.push_back(std::move(path[1]));
  }
  return res;
}

vector<string> GetLocalizedTypes(ftypes::BaseChecker const & checker, TypesHolder const & types)
{
  auto const & c = classif();
  vector<string> localized;
  for (auto const t : types)
    if (checker(t))
      localized.push_back(localisation::TranslatedFeatureType(c.GetReadableObjectName(t)));
  return localized;
}

class FeatureEstimator
{
  template <size_t N>
  static bool IsEqual(uint32_t t, uint32_t const (&arr)[N])
  {
    for (size_t i = 0; i < N; ++i)
      if (arr[i] == t)
        return true;
    return false;
  }

  static bool InSubtree(uint32_t t, uint32_t const orig)
  {
    ftype::TruncValue(t, ftype::GetLevel(orig));
    return t == orig;
  }

public:
  FeatureEstimator()
  {
    auto const & cl = classif();

    m_TypeContinent = cl.GetTypeByPath({"place", "continent"});
    m_TypeCountry = cl.GetTypeByPath({"place", "country"});

    m_TypeState = cl.GetTypeByPath({"place", "state"});
    m_TypeCounty[0] = cl.GetTypeByPath({"place", "region"});
    m_TypeCounty[1] = cl.GetTypeByPath({"place", "county"});

    m_TypeCity = cl.GetTypeByPath({"place", "city"});
    m_TypeTown = cl.GetTypeByPath({"place", "town"});

    m_TypeVillage[0] = cl.GetTypeByPath({"place", "village"});
    m_TypeVillage[1] = cl.GetTypeByPath({"place", "suburb"});

    m_TypeSmallVillage[0] = cl.GetTypeByPath({"place", "hamlet"});
    m_TypeSmallVillage[1] = cl.GetTypeByPath({"place", "locality"});
    m_TypeSmallVillage[2] = cl.GetTypeByPath({"place", "farm"});
  }

  void CorrectScaleForVisibility(TypesHolder const & types, int & scale) const
  {
    pair<int, int> const scaleR = GetDrawableScaleRangeForRules(types, RULE_ANY_TEXT);
    ASSERT_LESS_OR_EQUAL(scaleR.first, scaleR.second, ());

    // Result types can be without visible texts (matched by category).
    if (scaleR.first != -1)
    {
      if (scale < scaleR.first)
        scale = scaleR.first;
      else if (scale > scaleR.second)
        scale = scaleR.second;
    }
  }

  int GetViewportScale(TypesHolder const & types) const
  {
    int scale = GetDefaultScale();

    if (types.GetGeomType() == GeomType::Point)
      for (uint32_t t : types)
        scale = min(scale, GetScaleForType(t));

    CorrectScaleForVisibility(types, scale);
    return scale;
  }

private:
  static int GetDefaultScale() { return scales::GetUpperComfortScale(); }

  int GetScaleForType(uint32_t const type) const
  {
    if (type == m_TypeContinent)
      return 2;

    /// @todo Load countries bounding rects.
    if (type == m_TypeCountry)
      return 4;

    if (type == m_TypeState)
      return 6;

    if (IsEqual(type, m_TypeCounty))
      return 7;

    if (InSubtree(type, m_TypeCity))
      return 9;

    if (type == m_TypeTown)
      return 9;

    if (IsEqual(type, m_TypeVillage))
      return 12;

    if (IsEqual(type, m_TypeSmallVillage))
      return 14;

    return GetDefaultScale();
  }

  uint32_t m_TypeContinent;
  uint32_t m_TypeCountry;
  uint32_t m_TypeState;
  uint32_t m_TypeCounty[2];
  uint32_t m_TypeCity;
  uint32_t m_TypeTown;
  uint32_t m_TypeVillage[2];
  uint32_t m_TypeSmallVillage[3];
};

FeatureEstimator const & GetFeatureEstimator()
{
  static FeatureEstimator const featureEstimator;
  return featureEstimator;
}
}  // namespace

static constexpr std::string_view kStarSymbol = "★";
static constexpr std::string_view kMountainSymbol = "▲";
static constexpr std::string_view kDrinkingWaterYes = "🚰";
static constexpr std::string_view kDrinkingWaterNo = "🚱";

int GetFeatureViewportScale(TypesHolder const & types)
{
  return GetFeatureEstimator().GetViewportScale(types);
}

string const GetReadableAddress(string const & address)
{
  // Instead of housenumber range strings like 123:456, hyphenate like 123 - 456
  string out = address;
  size_t pos = 0;
  while ((pos = out.find(":", pos)) != string::npos)
  {
    out.replace(pos, 1, "\u2009\u2013\u2009");  // thin space + en-dash + thin space
    break;
  }
  return out;
}

/*
int8_t GetNameForSearchOnBooking(RegionData const & regionData, StringUtf8Multilang const & src, string & name)
{
  if (src.GetString(localisation::kDefaultNameIndex, name))
    return localisation::kDefaultNameIndex;

  vector<int8_t> mwmLangs;
  regionData.GetLanguages(mwmLangs);

  for (auto mwmLang : mwmLangs)
  {
    if (src.GetString(mwmLang, name))
      return mwmLang;
  }

  if (src.GetString(localisation::kEnglishLanguageIndex, name))
    return localisation::kEnglishLanguageIndex;

  name.clear();
  return localisation::kUnsupportedLanguageIndex;
}
*/

vector<string> GetLocalizedSubtypes(TypesHolder const & types)
{
  auto const & classificator = classif();
  auto subtypes = ftypes::Subtypes::Instance();
  vector<string> localizedSubtypes;
  for (auto const & type : types)
    if (subtypes.IsSubtype(type))
      localizedSubtypes.push_back(localisation::TranslatedFeatureType(classificator.GetReadableObjectName(type)));
  return localizedSubtypes;
}

vector<string> GetCuisines(TypesHolder const & types)
{
  auto const & isCuisine = ftypes::IsCuisineChecker::Instance();
  return GetRawTypeSecond(isCuisine, types);
}

vector<string> GetLocalizedCuisines(TypesHolder const & types)
{
  auto const & isCuisine = ftypes::IsCuisineChecker::Instance();
  return GetLocalizedTypes(isCuisine, types);
}

string GetLocalizedFeeType(TypesHolder const & types)
{
  auto const & isFeeType = ftypes::IsFeeTypeChecker::Instance();
  auto localized_types = GetLocalizedTypes(isFeeType, types);
  ASSERT_LESS_OR_EQUAL(localized_types.size(), 1, ());
  if (localized_types.empty())
    return "";
  return localized_types[0];
}

string GetReadableWheelchairType(TypesHolder const & types)
{
  auto const value = ftraits::Wheelchair::GetValue(types);
  if (!value.has_value())
    return "";

  switch (*value)
  {
  case ftraits::WheelchairAvailability::No: return "wheelchair-no";
  case ftraits::WheelchairAvailability::Yes: return "wheelchair-yes";
  case ftraits::WheelchairAvailability::Limited: return "wheelchair-limited";
  }
  UNREACHABLE();
}

std::optional<ftraits::WheelchairAvailability> GetWheelchairType(TypesHolder const & types)
{
  return ftraits::Wheelchair::GetValue(types);
}

bool HasAtm(TypesHolder const & types)
{
  auto const & isAtmType = ftypes::IsATMChecker::Instance();
  return isAtmType(types);
}

bool HasToilets(TypesHolder const & types)
{
  auto const & isToiletsType = ftypes::IsToiletsChecker::Instance();
  return isToiletsType(types);
}

string FormatDrinkingWater(TypesHolder const & types)
{
  auto const value = ftraits::DrinkingWater::GetValue(types);
  if (!value.has_value())
    return "";

  switch (*value)
  {
  case ftraits::DrinkingWaterAvailability::No: return std::string{kDrinkingWaterNo};
  case ftraits::DrinkingWaterAvailability::Yes: return std::string{kDrinkingWaterYes};
  }
  UNREACHABLE();
}

string FormatStars(uint8_t starsCount)
{
  std::string stars;
  for (int i = 0; i < starsCount && i < kMaxStarsCount; ++i)
    stars.append(kStarSymbol);
  return stars;
}

string FormatElevation(string_view elevation)
{
  if (!elevation.empty())
  {
    double value;
    if (strings::to_double(elevation, value))
      return std::string{kMountainSymbol} + platform::Distance::FormatAltitude(value);
    else
      LOG(LWARNING, ("Invalid elevation metadata:", elevation));
  }
  return {};
}

constexpr char const * kWlan = "wlan";
constexpr char const * kWired = "wired";
constexpr char const * kTerminal = "terminal";
constexpr char const * kYes = "yes";
constexpr char const * kNo = "no";
constexpr char const * kOnly = "only";

string DebugPrint(Internet internet)
{
  switch (internet)
  {
  case Internet::No: return kNo;
  case Internet::Yes: return kYes;
  case Internet::Wlan: return kWlan;
  case Internet::Wired: return kWired;
  case Internet::Terminal: return kTerminal;
  case Internet::Unknown: break;
  }
  return {};
}

Internet InternetFromString(std::string_view inet)
{
  if (inet.empty())
    return Internet::Unknown;
  if (inet.find(kWlan) != string::npos)
    return Internet::Wlan;
  if (inet.find(kWired) != string::npos)
    return Internet::Wired;
  if (inet.find(kTerminal) != string::npos)
    return Internet::Terminal;
  if (inet == kYes)
    return Internet::Yes;
  if (inet == kNo)
    return Internet::No;
  return Internet::Unknown;
}

YesNoUnknown YesNoUnknownFromString(std::string_view str)
{
  if (str.empty())
    return Unknown;
  if (str.find(kOnly) != string::npos)
    return Yes;
  if (str.find(kYes) != string::npos)
    return Yes;
  if (str.find(kNo) != string::npos)
    return No;
  else
    return YesNoUnknown::Unknown;
}

}  // namespace feature
