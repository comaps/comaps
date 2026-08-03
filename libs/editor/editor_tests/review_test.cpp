#include "testing/testing.hpp"

#include "editor/review.hpp"

#include "geometry/point2d.hpp"

#include "i18n/localisation.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/map_object.hpp"

#include <optional>
#include <string>
#include <vector>

namespace
{
using namespace reviews::internal;

class TestMapObject : public osm::MapObject
{
public:
  void SetDefaultName(std::string const & name) { m_name.AddString(localisation::kDefaultNameIndex, name); }
  void SetGeomType(feature::GeomType const geomType) { m_geomType = geomType; }
  void SetMercator(m2::PointD mercator) { m_mercator = mercator; }
  void SetPoints(std::vector<m2::PointD> points) { m_points = std::move(points); }
  void SetTriangles(std::vector<m2::PointD> triangles) { m_triangles = std::move(triangles); }
};

UNIT_TEST(Review_UncertaintyPoint)
{
  TestMapObject mo;
  mo.SetGeomType(feature::GeomType::Point);
  auto uncertainty = Uncertainty(mo);
  TEST_EQUAL(uncertainty, 10, ());
}

UNIT_TEST(Review_UncertaintyLine)
{
  TestMapObject mo;
  // a line of 0.001 degree length centered at 0,0
  mo.SetGeomType(feature::GeomType::Line);
  mo.SetPoints({m2::PointD(0, -0.0005), {m2::PointD(0, 0.0005)}});
  mo.SetMercator(m2::PointD(0, 0));
  auto uncertainty = Uncertainty(mo);
  // 0.0005 degree lon on the equator is 55 m approx
  TEST_ALMOST_EQUAL_ABS(static_cast<double>(uncertainty), 55.0, 5.0, ());
}

UNIT_TEST(Review_UncertaintyArea)
{
  TestMapObject mo;
  // a "rectangle" of 0.001 degrees x 0.002 degrees centered at 0,0
  mo.SetGeomType(feature::GeomType::Area);
  mo.SetTriangles({m2::PointD(-0.0005, -0.001), m2::PointD(-0.0005, 0.001), m2::PointD(0.0005, 0.001),
                   m2::PointD(-0.0005, -0.001), m2::PointD(0.0005, 0.001), m2::PointD(0.0005, -0.001)});
  mo.SetMercator(m2::PointD(0, 0));
  auto uncertainty = Uncertainty(mo);
  // 0,0 to 0.0005,0.001 is 125 m approx
  TEST_ALMOST_EQUAL_ABS(static_cast<double>(uncertainty), 125.0, 5.0, ());
}

UNIT_TEST(Review_MangroveUrlGeoUriEncoding)
{
  TestMapObject mo;
  mo.SetGeomType(feature::GeomType::Point);
  mo.SetMercator(m2::PointD(0, 0));

  // UTF-8 name
  {
    mo.SetDefaultName("Välaberget");
    auto mangroveUrl = Mangrove::EditorUrl(mo);
    // both encoding once and twice works in this case
    auto encodedOnce = std::optional("https://mangrove.reviews/search?sub=geo%3A0%2C0%3Fq%3DV%C3%A4laberget%26u%3D10");
    auto encodedTwice =
        std::optional("https://mangrove.reviews/search?sub=geo%3A0%2C0%3Fq%3DV%25C3%25A4laberget%26u%3D10");
    TEST(mangroveUrl == encodedOnce || mangroveUrl == encodedTwice,
         ("actual =", mangroveUrl, " expected one of:", encodedOnce, encodedTwice));
  }

  // a name with '&'
  {
    mo.SetDefaultName("Casper East RV Park & Campground");
    auto mangroveUrl = Mangrove::EditorUrl(mo);
    // encoding twice is required to properly handle the ampersand
    auto encodedTwice = std::optional(
        "https://mangrove.reviews/"
        "search?sub=geo%3A0%2C0%3Fq%3DCasper%2520East%2520RV%2520Park%2520%2526%2520Campground%26u%3D10");
    TEST_EQUAL(mangroveUrl, encodedTwice, ());
  }
}
}  // namespace
