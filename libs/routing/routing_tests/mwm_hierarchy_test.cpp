#include "testing/testing.hpp"

#include "routing/mwm_hierarchy_handler.hpp"
#include "routing/routing_options.hpp"

#include "storage/country_parent_getter.hpp"
#include "storage/routing_helpers.hpp"

namespace mwm_hierarchy_test
{
using namespace routing;

UNIT_TEST(CountryParentGetter_Smoke)
{
  storage::CountryParentGetter getter;
  TEST_EQUAL(getter("Belarus_Hrodna Region"), "Belarus", ());
  TEST_EQUAL(getter("Russia_Arkhangelsk Oblast_Central"), "Russian Federation", ());
  TEST_EQUAL(getter("Crimea"), "", ());

  TEST_EQUAL(getter("Israel"), "Israel Region", ());
  TEST_EQUAL(getter("Palestine"), "Palestine Region", ());
  TEST_EQUAL(getter("Jerusalem"), "", ());
  TEST_EQUAL(getter("US_New York_New York"), "New York", ());
  TEST_EQUAL(getter("UK_England_West Midlands"), "United Kingdom", ());
}

uint16_t GetCountryID(std::shared_ptr<NumMwmIds> const & mwmIDs, std::string mwmName)
{
  return mwmIDs->GetId(platform::CountryFile(std::move(mwmName)));
}

UNIT_TEST(MwmHierarchyHandler_Smoke)
{
  storage::CountryParentGetter getter;
  auto mwmIDs = CreateNumMwmIds(getter.GetStorageForTesting());
  routing::MwmHierarchyHandler handler(mwmIDs, getter);

  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Maglieu Region"),
                                      GetCountryID(mwmIDs, "Belarus_Vitebsk Region")),
       ());
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"),
                                     GetCountryID(mwmIDs, "Lithuania_East")),
       ());
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Maglieu Region"),
                                      GetCountryID(mwmIDs, "Russia_Smolensk Oblast")),
       ());
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Ukraine_Kherson Oblast"), GetCountryID(mwmIDs, "Crimea")),
       ());
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Russia_Krasnodar Krai"), GetCountryID(mwmIDs, "Crimea")),
       ());
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Denmark_Region Zealand"),
                                      GetCountryID(mwmIDs, "Denmark_Region of Southern Denmark")),
       ());

  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Ukraine_Zakarpattia Oblast"),
                                     GetCountryID(mwmIDs, "Slovakia_Region of Kosice")),
       ());
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Ukraine_Zakarpattia Oblast"),
                                     GetCountryID(mwmIDs, "Hungary_Northern Great Plain")),
       ());
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Hungary_Northern Great Plain"),
                                      GetCountryID(mwmIDs, "Slovakia_Region of Kosice")),
       ());

  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Ireland_Connacht"),
                                      GetCountryID(mwmIDs, "UK_Northern Ireland")),
       ());
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Ireland_Leinster"), GetCountryID(mwmIDs, "UK_Wales")), ());

  char const * ip[] = {"Israel", "Jerusalem", "Palestine"};
  for (auto s1 : ip)
    for (auto s2 : ip)
      TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, s1), GetCountryID(mwmIDs, s2)), (s1, s2));
}

UNIT_TEST(MwmHierarchyHandler_BorderAvoidance_Any)
{
  storage::CountryParentGetter getter;
  auto mwmIDs = CreateNumMwmIds(getter.GetStorageForTesting());

  BorderAvoidanceSettings settings;
  settings.SetMode(BorderAvoidance::Any);
  routing::MwmHierarchyHandler handler(mwmIDs, getter, settings);

  // Same MWM: no penalty.
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"),
                                      GetCountryID(mwmIDs, "Belarus_Hrodna Region")),
       ());

  // Same country: no penalty.
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Maglieu Region"),
                                      GetCountryID(mwmIDs, "Belarus_Vitebsk Region")),
       ());

  // ANY cross-border: penalty always.
  // Schengen internal (Denmark <-> France): still penalized with Any mode.
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Denmark_Region Zealand"),
                                     GetCountryID(mwmIDs, "France_Alsace")),
       ());
  // EAEU internal (Belarus <-> Russia): still penalized with Any mode.
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"),
                                     GetCountryID(mwmIDs, "Russia_Smolensk Oblast")),
       ());
  // Regular cross-border: penalized.
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"),
                                     GetCountryID(mwmIDs, "Lithuania_East")),
       ());

  // Ireland <-> Northern Ireland: penalized with Any mode.
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Ireland_Connacht"),
                                     GetCountryID(mwmIDs, "UK_Northern Ireland")),
       ());

  // Penalty should be hard (very high).
  RouteWeight const penalty = handler.GetCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"),
                                                            GetCountryID(mwmIDs, "Lithuania_East"));
  RouteWeight const softPenalty =
      routing::MwmHierarchyHandler(mwmIDs, getter)
          .GetCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"), GetCountryID(mwmIDs, "Lithuania_East"));
  TEST(penalty > softPenalty, ("Hard penalty should be greater than soft penalty"));
}

UNIT_TEST(MwmHierarchyHandler_BorderAvoidance_NonInternal)
{
  storage::CountryParentGetter getter;
  auto mwmIDs = CreateNumMwmIds(getter.GetStorageForTesting());

  BorderAvoidanceSettings settings;
  settings.SetMode(BorderAvoidance::NonInternal);
  routing::MwmHierarchyHandler handler(mwmIDs, getter, settings);

  // NonInternal mode: hard penalty for non-Schengen/EAEU borders.
  // Schengen internal: no penalty.
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Denmark_Region Zealand"),
                                      GetCountryID(mwmIDs, "Denmark_Region of Southern Denmark")),
       ());
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Hungary_Northern Great Plain"),
                                      GetCountryID(mwmIDs, "Slovakia_Region of Kosice")),
       ());

  // EAEU internal: no penalty.
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Maglieu Region"),
                                      GetCountryID(mwmIDs, "Russia_Smolensk Oblast")),
       ());

  // Non-Schengen/EAEU cross-border: penalty (hard).
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"),
                                     GetCountryID(mwmIDs, "Lithuania_East")),
       ());

  // Verify hard penalty is used for NonInternal.
  RouteWeight const penalty = handler.GetCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"),
                                                            GetCountryID(mwmIDs, "Lithuania_East"));
  TEST(penalty.GetWeight() > 3600, ("NonInternal should use hard penalty"));

  // Same country: no penalty.
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Maglieu Region"),
                                      GetCountryID(mwmIDs, "Belarus_Vitebsk Region")),
       ());
}

UNIT_TEST(MwmHierarchyHandler_BorderAvoidance_Specific)
{
  storage::CountryParentGetter getter;
  auto mwmIDs = CreateNumMwmIds(getter.GetStorageForTesting());

  BorderAvoidanceSettings settings;
  settings.SetMode(BorderAvoidance::Specific);
  settings.SetAvoidedCountries({"Lithuania"});
  routing::MwmHierarchyHandler handler(mwmIDs, getter, settings);

  // Belarus <-> Lithuania: penalty (Lithuania is in avoided set).
  TEST(handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"),
                                     GetCountryID(mwmIDs, "Lithuania_East")),
       ());

  // Belarus <-> Russia: no penalty (Russia not in avoided set).
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"),
                                      GetCountryID(mwmIDs, "Russia_Smolensk Oblast")),
       ());

  // Denmark <-> Hungary: no penalty (neither in avoided set, both Schengen).
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Denmark_Region Zealand"),
                                      GetCountryID(mwmIDs, "Hungary_Northern Great Plain")),
       ());

  // Slovakia <-> Hungary: no penalty (neither in avoided set, both Schengen).
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Slovakia_Region of Kosice"),
                                      GetCountryID(mwmIDs, "Hungary_Northern Great Plain")),
       ());

  // Same country: no penalty.
  TEST(!handler.HasCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Maglieu Region"),
                                      GetCountryID(mwmIDs, "Belarus_Vitebsk Region")),
       ());

  // Penalty for avoided country crossing should be hard.
  RouteWeight const penalty = handler.GetCrossBorderPenalty(GetCountryID(mwmIDs, "Belarus_Hrodna Region"),
                                                            GetCountryID(mwmIDs, "Lithuania_East"));
  TEST(penalty.GetWeight() > 3600, ("Penalty should be greater than 1 hour"));
}

UNIT_TEST(BorderAvoidanceSettings_Persistence)
{
  // Save original state.
  auto const saved = BorderAvoidanceSettings::LoadFromSettings();

  // Test round-trip for Specific mode with countries.
  BorderAvoidanceSettings settings;
  settings.SetMode(BorderAvoidance::Specific);
  settings.SetAvoidedCountries({"Lithuania", "Poland", "Germany"});
  settings.SaveToSettings();

  auto const loaded = BorderAvoidanceSettings::LoadFromSettings();
  TEST_EQUAL(loaded.GetMode(), BorderAvoidance::Specific, ());
  TEST_EQUAL(loaded.GetAvoidedCountries().size(), 3, ());
  TEST(loaded.GetAvoidedCountries().contains("Lithuania"), ());
  TEST(loaded.GetAvoidedCountries().contains("Poland"), ());
  TEST(loaded.GetAvoidedCountries().contains("Germany"), ());

  // Test round-trip for Any mode.
  BorderAvoidanceSettings settings2;
  settings2.SetMode(BorderAvoidance::Any);
  settings2.SaveToSettings();

  auto const loaded2 = BorderAvoidanceSettings::LoadFromSettings();
  TEST_EQUAL(loaded2.GetMode(), BorderAvoidance::Any, ());
  TEST(loaded2.GetAvoidedCountries().empty(), ());

  // Restore original state.
  saved.SaveToSettings();
}

UNIT_TEST(BorderAvoidanceSettings_RoundTrip_None)
{
  auto const saved = BorderAvoidanceSettings::LoadFromSettings();

  BorderAvoidanceSettings settings;
  settings.SetMode(BorderAvoidance::None);
  settings.SaveToSettings();

  auto const loaded = BorderAvoidanceSettings::LoadFromSettings();
  TEST_EQUAL(loaded.GetMode(), BorderAvoidance::None, ());
  TEST(loaded.GetAvoidedCountries().empty(), ());
  TEST(loaded.IsEmpty(), ());

  saved.SaveToSettings();
}

UNIT_TEST(BorderAvoidance_ToString_FromString)
{
  TEST_EQUAL(ToString(BorderAvoidance::None), "none", ());
  TEST_EQUAL(ToString(BorderAvoidance::Any), "any", ());
  TEST_EQUAL(ToString(BorderAvoidance::NonInternal), "non_internal", ());
  TEST_EQUAL(ToString(BorderAvoidance::Specific), "specific", ());

  TEST_EQUAL(BorderAvoidanceFromString("none"), BorderAvoidance::None, ());
  TEST_EQUAL(BorderAvoidanceFromString("any"), BorderAvoidance::Any, ());
  TEST_EQUAL(BorderAvoidanceFromString("non_internal"), BorderAvoidance::NonInternal, ());
  TEST_EQUAL(BorderAvoidanceFromString("specific"), BorderAvoidance::Specific, ());
  TEST_EQUAL(BorderAvoidanceFromString("unknown"), BorderAvoidance::None, ());
  TEST_EQUAL(BorderAvoidanceFromString(""), BorderAvoidance::None, ());
}

}  // namespace mwm_hierarchy_test
