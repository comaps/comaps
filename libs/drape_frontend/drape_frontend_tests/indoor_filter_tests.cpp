#include "testing/testing.hpp"

#include "drape_frontend/indoor_filter.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

UNIT_TEST(IndoorFilter_ShouldSkipIndoorFeature)
{
  classificator::Load();
  auto const & cl = classif();

  feature::TypesHolder indoorRoom;
  indoorRoom.Add(cl.GetTypeByPath({"indoor", "room"}));

  feature::TypesHolder building;
  building.Add(cl.GetTypeByPath({"building"}));

  // Non-indoor features are never skipped, whatever the level.
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
}
