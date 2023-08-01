#include "testing/testing.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_altitude.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"

#include "routing/routing_helpers.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "platform/local_country_file.hpp"

#include "base/math.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace get_altitude_tests
{
using namespace feature;
using namespace geometry;
using namespace platform;
using namespace std;

class FeaturesGuard
{
public:
  FrozenDataSource m_dataSource;
  MwmSet::MwmHandle m_handle;
  unique_ptr<AltitudeLoaderCached> m_altitudes;

  explicit FeaturesGuard(string const & countryId)
  {
    LocalCountryFile const country = integration::GetLocalCountryFileByCountryId(CountryFile(countryId));
    TEST_NOT_EQUAL(country, LocalCountryFile(), ());
    TEST(country.HasFiles(), (country));

    pair<MwmSet::MwmId, MwmSet::RegResult> const res = m_dataSource.RegisterMap(country);
    TEST_EQUAL(res.second, MwmSet::RegResult::Success, ());
    m_handle = m_dataSource.GetMwmHandleById(res.first);
    TEST(m_handle.IsAlive(), ());
    TEST(GetValue(), ());

    m_altitudes = make_unique<AltitudeLoaderCached>(*GetValue());
  }

  MwmValue const * GetValue() { return m_handle.GetValue(); }
};

void TestAltitudeOfAllMwmFeatures(string const & countryId,
                                  Altitude const altitudeLowerBoundMeters,
                                  Altitude const altitudeUpperBoundMeters)
{
  FeaturesGuard features(countryId);

  ForEachFeature(features.GetValue()->m_cont, [&](FeatureType & f, uint32_t const & id)
  {
    if (!routing::IsRoad(TypesHolder(f)))
      return;

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = f.GetPointsCount();
    if (pointsCount == 0)
      return;

    auto const & altitudes = features.m_altitudes->GetAltitudes(id, pointsCount);
    TEST(!altitudes.empty(),
         ("Empty altitude vector. MWM:", countryId, ", feature id:", id, ", altitudes:", altitudes));

    for (auto const altitude : altitudes)
    {
      TEST_EQUAL(base::Clamp(altitude, altitudeLowerBoundMeters, altitudeUpperBoundMeters), altitude,
                 ("Unexpected altitude. MWM:", countryId, ", feature id:", id, ", altitudes:", altitudes));
    }
  });
}

UNIT_TEST(GetAltitude_AllMwmFeaturesTest)
{
  classificator::Load();

  TestAltitudeOfAllMwmFeatures("Russia_Moscow", 50 /* altitudeLowerBoundMeters */,
                               300 /* altitudeUpperBoundMeters */);
  TestAltitudeOfAllMwmFeatures("Nepal_Kathmandu", 250 /* altitudeLowerBoundMeters */,
                               6000 /* altitudeUpperBoundMeters */);
  TestAltitudeOfAllMwmFeatures("Netherlands_North Holland_Amsterdam", -25 /* altitudeLowerBoundMeters */,
                               50 /* altitudeUpperBoundMeters */);
}

/*
void PrintGeometryAndAltitude(std::string const & countryID, ms::LatLon const & ll, double distM)
{
  FeaturesGuard features(countryID);
  auto const point = mercator::FromLatLon(ll);
  m2::RectD const rect = mercator::RectByCenterXYAndSizeInMeters(point, distM);

  features.m_dataSource.ForEachInRect([&](FeatureType & ft)
  {
    if (!routing::IsRoad(TypesHolder(ft)))
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = ft.GetPointsCount();
    if (pointsCount == 0)
      return;

    if (GetMinDistanceMeters(ft, point) > distM)
      return;

    stringstream geomSS;
    geomSS.precision(20);
    for (size_t i = 0; i < pointsCount; ++i)
    {
      auto const ll = mercator::ToLatLon(ft.GetPoint(i));
      geomSS << "{ " << ll.m_lat << ", " << ll.m_lon << " }, ";
    }
    LOG(LINFO, (geomSS.str()));

    auto const & altitudes = features.m_altitudes->GetAltitudes(ft.GetID().m_index, pointsCount);
    LOG(LINFO, (ft.GetName(StringUtf8Multilang::kDefaultCode), altitudes));

  }, rect, scales::GetUpperScale());
}

UNIT_TEST(GetAltitude_SamplesTest)
{
  classificator::Load();

  PrintGeometryAndAltitude("Italy_Lazio", {41.8998667, 12.4985937}, 15.0);
  PrintGeometryAndAltitude("Crimea", { 44.7598876, 34.3160482 }, 5.0);
}
*/

}  // namespace get_altitude_tests
