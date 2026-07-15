#include "testing/testing.hpp"

#include "drape_frontend/indoor_filter.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/indoor_level.hpp"

UNIT_TEST(IndoorFilter_ShouldSkipIndoorFeature)
{
  classificator::Load();
  auto const & cl = classif();

  feature::TypesHolder indoorRoom;
  indoorRoom.Add(cl.GetTypeByPath({"indoor", "room"}));

  feature::TypesHolder building;
  building.Add(cl.GetTypeByPath({"building"}));

  feature::TypesHolder poi;
  poi.Add(cl.GetTypeByPath({"amenity", "cafe"}));

  feature::TypesHolder steps;
  steps.Add(cl.GetTypeByPath({"highway", "steps"}));

  // Buildings/areas are not floor-bound and are never skipped, whatever the level.
  TEST(!df::ShouldSkipIndoorFeature(building, "", 0.0), ());
  TEST(!df::ShouldSkipIndoorFeature(building, "5", 0.0), ());

  // Indoor features are kept only on their level(s).
  TEST(!df::ShouldSkipIndoorFeature(indoorRoom, "0;1", 0.0), ());
  TEST(!df::ShouldSkipIndoorFeature(indoorRoom, "0;1", 1.0), ());
  TEST(df::ShouldSkipIndoorFeature(indoorRoom, "0;1", 2.0), ());

  // Missing level metadata means level 0.
  TEST(!df::ShouldSkipIndoorFeature(indoorRoom, "", 0.0), ());
  TEST(df::ShouldSkipIndoorFeature(indoorRoom, "", 1.0), ());

  // Fractional levels.
  TEST(!df::ShouldSkipIndoorFeature(indoorRoom, "1.5", 1.5), ());
  TEST(df::ShouldSkipIndoorFeature(indoorRoom, "1.5", 1.0), ());

  // A POI carrying a level=* tag is treated as floor-bound and hidden off its floor.
  TEST(!df::ShouldSkipIndoorFeature(poi, "2", 2.0), ());
  TEST(df::ShouldSkipIndoorFeature(poi, "2", 1.0), ());
  // A POI with no level tag is not floor-bound and is always shown.
  TEST(!df::ShouldSkipIndoorFeature(poi, "", 1.0), ());

  // Non-POI features carrying a level tag (stairs, elevators, footways) are floor-bound too.
  TEST(!df::ShouldSkipIndoorFeature(steps, "0;1", 1.0), ());
  TEST(df::ShouldSkipIndoorFeature(steps, "0;1", 2.0), ());
  // ...but with no level tag they are ordinary map features and always shown.
  TEST(!df::ShouldSkipIndoorFeature(steps, "", 2.0), ());

  // When no level is active (selector inactive / panned away), nothing is skipped,
  // so level-tagged POIs and indoor features stay visible in the ordinary map.
  TEST(!df::ShouldSkipIndoorFeature(poi, "2", indoor::kNoActiveLevel), ());
  TEST(!df::ShouldSkipIndoorFeature(indoorRoom, "0;1", indoor::kNoActiveLevel), ());
}
