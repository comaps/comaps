#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/indoor_level.hpp"
#include "indexer/scales.hpp"

#include "platform/local_country_file.hpp"

#include "geometry/mercator.hpp"

#include <map>
#include <string>

UNIT_TEST(MallDiag_IndoorScan)
{
  Framework frm(FrameworkParams(false /* m_enableDiffs */));
  frm.DeregisterAllMaps();
  frm.RegisterMap(platform::LocalCountryFile::MakeForTesting("MallOfAmerica"));

  // Exact viewport rect observed on the Android emulator.
  m2::RectD const rect(m2::PointD(-93.243948392868048813, 50.291980919610800527),
                       m2::PointD(-93.241051607131964829, 50.29793542362386205));

  size_t total = 0, indoor = 0;
  std::map<std::string, size_t> indoorTypes;
  std::map<std::string, size_t> levels;

  frm.GetDataSource().ForEachInRect([&](FeatureType & ft)
  {
    ++total;
    feature::TypesHolder const types(ft);
    if (!ftypes::IsIndoorChecker::Instance()(types))
      return;
    ++indoor;
    for (uint32_t t : types)
      ++indoorTypes[classif().GetFullObjectName(t)];
    levels[std::string(ft.GetMetadata(feature::Metadata::FMD_LEVEL))]++;
  }, rect, scales::GetUpperScale());

  LOG(LINFO, ("MallDiag: total features =", total, "indoor =", indoor));
  for (auto const & [type, count] : indoorTypes)
    LOG(LINFO, ("MallDiag: type", type, "x", count));
  for (auto const & [level, count] : levels)
    LOG(LINFO, ("MallDiag: level '", level, "' x", count));

  TEST_GREATER(indoor, 0, ());
}
