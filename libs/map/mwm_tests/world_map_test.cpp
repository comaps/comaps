#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "i18n/localisation.hpp"

#include <iostream>
#include <map>
#include <set>

UNIT_TEST(World_Capitals)
{
  classificator::Load();
  auto const capitalType = classif().GetTypeByPath({"place", "city", "capital", "2"});
  std::map<std::string_view, ms::LatLon> const testCapitals = {
      {"Lisbon", {38.7077507, -9.1365919}},
      {"Warsaw", {52.2319581, 21.0067249}},
      {"Kyiv", {50.4500336, 30.5241361}},
      {"Roseau", {15.2991923, -61.3872868}},
  };
  std::set<std::string_view> foundCapitals;

  platform::LocalCountryFile localFile(platform::LocalCountryFile::MakeForTesting(WORLD_FILE_NAME));

  FrozenDataSource dataSource;
  auto const res = dataSource.RegisterMap(localFile);
  TEST_EQUAL(res.second, MwmSet::RegResult::Success, ());

  size_t capitalsCount = 0;

  FeaturesLoaderGuard guard(dataSource, res.first);
  size_t const count = guard.GetNumFeatures();
  for (size_t id = 0; id < count; ++id)
  {
    auto ft = guard.GetFeatureByIndex(id);
    if (ft->GetGeomType() != feature::GeomType::Point)
      continue;

    bool found = false;
    ft->ForEachType([&found, capitalType](uint32_t t)
    {
      if (t == capitalType)
        found = true;
    });

    if (!found)
      continue;

    ++capitalsCount;

    auto const it = testCapitals.find(ft->GetName(localisation::kEnglishLanguageIndex));
    if (it != testCapitals.end() && ms::DistanceOnEarth(mercator::ToLatLon(ft->GetCenter()), it->second) < 20000)
      foundCapitals.insert(it->first);
  }

  TEST_EQUAL(foundCapitals.size(), testCapitals.size(), (foundCapitals));

  // Got 225 values from the first launch. May vary slightly ..
  TEST_GREATER_OR_EQUAL(capitalsCount, 215, ());
}
