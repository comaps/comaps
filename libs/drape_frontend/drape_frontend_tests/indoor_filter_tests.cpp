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

  // Helper geometry: a point-sized feature rect at the origin, a nearby polygon that contains it,
  // and a far polygon that doesn't overlap anything below.
  m2::RectD const poiRect(0, 0, 0, 0);
  m2::RectD const nearbyPoly(-0.001, -0.001, 0.001, 0.001);
  m2::RectD const farPoly(1.0, 1.0, 2.0, 2.0);

  // Buildings/areas are not floor-bound and are never skipped, whatever the level.
  TEST(!df::ShouldSkipIndoorFeature(building, "", 0.0, {}, {}, {}), ());
  TEST(!df::ShouldSkipIndoorFeature(building, "5", 0.0, {}, {}, {}), ());

  // Indoor features are kept only on their level(s).  Proximity/polyRects not consulted for isIndoor.
  TEST(!df::ShouldSkipIndoorFeature(indoorRoom, "0;1", 0.0, {}, {}, {}), ());
  TEST(!df::ShouldSkipIndoorFeature(indoorRoom, "0;1", 1.0, {}, {}, {}), ());
  TEST(df::ShouldSkipIndoorFeature(indoorRoom, "0;1", 2.0, {}, {}, {}), ());

  // Missing level metadata means level 0.
  TEST(!df::ShouldSkipIndoorFeature(indoorRoom, "", 0.0, {}, {}, {}), ());
  TEST(df::ShouldSkipIndoorFeature(indoorRoom, "", 1.0, {}, {}, {}), ());

  // Fractional levels.
  TEST(!df::ShouldSkipIndoorFeature(indoorRoom, "1.5", 1.5, {}, {}, {}), ());
  TEST(df::ShouldSkipIndoorFeature(indoorRoom, "1.5", 1.0, {}, {}, {}), ());

  // A POI near an indoor building is filtered by its level tag.
  TEST(!df::ShouldSkipIndoorFeature(poi, "2", 2.0, poiRect, {nearbyPoly}, {1.0, 2.0}), ());
  TEST(df::ShouldSkipIndoorFeature(poi, "2", 1.0, poiRect, {nearbyPoly}, {1.0, 2.0}), ());

  // Proximity gate: a POI far from any indoor polygon is NOT filtered, even if its level tag matches.
  TEST(!df::ShouldSkipIndoorFeature(poi, "2", 1.0, poiRect, {farPoly}, {1.0, 2.0}), ());

  // A large non-indoor polygon (e.g. a park or parking lot tagged level=1) whose center is far from
  // the indoor polygon, but whose edge touches it, must still be treated as near. Center-only
  // proximity would miss this: the two rects' centers are ~1.5 units apart while their edges touch
  // at x = 0.001.
  m2::RectD const bigAdjacentPoly(0.001, -1.0, 3.0, 1.0);
  TEST_GREATER(nearbyPoly.Center().Length(bigAdjacentPoly.Center()), 1.0, ());
  TEST(df::ShouldSkipIndoorFeature(poi, "2", 1.0, bigAdjacentPoly, {nearbyPoly}, {1.0, 2.0}), ());

  // A POI with no level tag is not floor-bound and is always shown.
  TEST(!df::ShouldSkipIndoorFeature(poi, "", 1.0, {}, {}, {}), ());

  // Non-POI features carrying a level tag (stairs, elevators, footways) are floor-bound too.
  TEST(!df::ShouldSkipIndoorFeature(steps, "0;1", 1.0, poiRect, {nearbyPoly}, {0.0, 1.0}), ());
  TEST(df::ShouldSkipIndoorFeature(steps, "0;1", 2.0, poiRect, {nearbyPoly}, {0.0, 1.0}), ());
  // ...but with no level tag they are ordinary map features and always shown.
  TEST(!df::ShouldSkipIndoorFeature(steps, "", 2.0, {}, {}, {}), ());

  // Level-in-building gate: a feature whose level is NOT among the building's floors is left
  // visible even in indoor mode. This prevents a transit platform at level=-1 from being hidden
  // when viewing a nearby mall with floors 0-3.
  TEST(!df::ShouldSkipIndoorFeature(poi, "-1", 1.0, poiRect, {nearbyPoly}, {0.0, 1.0, 2.0, 3.0}), ());

  // No active level hides indoor.
  TEST(df::ShouldSkipIndoorFeature(indoorRoom, "0;1", indoor::kNoActiveLevel, {}, {}, {}), ());
  // Others still visible.
  TEST(!df::ShouldSkipIndoorFeature(poi, "2", indoor::kNoActiveLevel, {}, {}, {}), ());
  TEST(!df::ShouldSkipIndoorFeature(building, "5", indoor::kNoActiveLevel, {}, {}, {}), ());
}
