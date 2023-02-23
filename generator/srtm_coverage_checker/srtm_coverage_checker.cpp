#include "generator/srtm_parser.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point_with_altitude.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <iostream>
#include <mutex>
#include <vector>

#include <gflags/gflags.h>

DEFINE_string(srtm_path, "", "Path to directory with SRTM files");
DEFINE_string(mwm_path, "", "Path to mwm files (writable dir)");
DEFINE_bool(check_dist, false, "Check feature sections distance");

class SafeTileManager
{
  generator::SrtmTileManager m_manager;
  std::mutex m_mutex;

  uint32_t m_ferry;

public:
  explicit SafeTileManager(std::string const & dir) : m_manager(dir)
  {
    m_ferry = classif().GetTypeByPath({"route", "ferry"});
    CHECK(m_ferry != Classificator::INVALID_TYPE, ());
  }

  bool IsAltitudeRoad(FeatureType & ft) const
  {
    feature::TypesHolder types(ft);
    return (routing::IsRoad(types) && !types.Has(m_ferry));
  }

  geometry::Altitude GetAltitude(ms::LatLon const & coord)
  {
    std::lock_guard guard(m_mutex);
    return m_manager.GetAltitude(coord);
  }

  void Purge()
  {
    std::lock_guard guard(m_mutex);
    m_manager.Purge();
  }
};

template <class FnT> void ForEachMWM(SafeTileManager & manager, FnT && fn)
{
  std::vector<platform::LocalCountryFile> localFiles;
  FindAllLocalMapsAndCleanup(std::numeric_limits<int64_t>::max() /* latestVersion */, localFiles);

  // Better use ComputationalThreadPool, but we want to call SafeTileManager::Purge after each batch.
  size_t constexpr kThreadsCount = 24;
  std::vector<std::thread> pool;

  size_t workers = 0;
  for (auto & file : localFiles)
  {
    // Skip worlds.
    if (file.GetDirectory().empty() || file.GetCountryName().starts_with("World"))
      continue;

    file.SyncWithDisk();
    if (!file.OnDisk(MapFileType::Map))
    {
      LOG_SHORT(LWARNING, ("Map file not found for:", file.GetCountryName()));
      continue;
    }

    LOG_SHORT(LINFO, ("Processing", file.GetCountryName()));

    pool.emplace_back([&fn, &file]() { fn(file); });

    if (++workers == kThreadsCount)
    {
      for (auto & t : pool)
        t.join();
      pool.clear();

      manager.Purge();
      workers = 0;
    }
  }

  for (auto & t : pool)
    t.join();
}

void CheckCoverage(SafeTileManager & manager)
{
  ForEachMWM(manager, [&](platform::LocalCountryFile const & file)
  {
    size_t all = 0;
    size_t good = 0;
    feature::ForEachFeature(file.GetPath(MapFileType::Map), [&](FeatureType & ft, uint32_t)
    {
      if (!manager.IsAltitudeRoad(ft))
        return;

      ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
      all += ft.GetPointsCount();

      for (size_t i = 0; i < ft.GetPointsCount(); ++i)
      {
        auto const height = manager.GetAltitude(mercator::ToLatLon(ft.GetPoint(i)));
        if (height != geometry::kInvalidAltitude)
          good++;
      }
    });

    auto const bad = all - good;
    auto const percent = (all == 0) ? 0.0 : bad * 100.0 / all;
    LOG_SHORT(LINFO, (percent > 10.0 ? "Huge" : "Low", "error rate in:", file.GetCountryName(),
              "good:", good, "bad:", bad, "all:", all, "%:", percent));
  });
}

void CheckDistance(SafeTileManager & manager)
{
  ForEachMWM(manager, [&](platform::LocalCountryFile const & file)
  {
    size_t all = 0;
    size_t added = 0;
    size_t invalid = 0;
    feature::ForEachFeature(file.GetPath(MapFileType::Map), [&](FeatureType & ft, uint32_t)
    {
      if (!manager.IsAltitudeRoad(ft))
        return;

      ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
      all += ft.GetPointsCount();

      for (size_t i = 1; i < ft.GetPointsCount(); ++i)
      {
        auto const ll1 = mercator::ToLatLon(ft.GetPoint(i-1));
        auto const alt1 = manager.GetAltitude(ll1);
        auto const ll2 = mercator::ToLatLon(ft.GetPoint(i));
        auto const alt2 = manager.GetAltitude(ll2);

        if (alt1 == geometry::kInvalidAltitude || alt2 == geometry::kInvalidAltitude)
        {
          ++invalid;
          continue;
        }

        // Divide by 1 second sections.
        size_t const sections = std::round(ms::DistanceOnSphere(ll1.m_lat, ll1.m_lon, ll2.m_lat, ll2.m_lon) * 3600);
        if (sections < 2)
          continue;

        for (size_t j = 1; j < sections; ++j)
        {
          double const a = j / double(sections);
          ms::LatLon const ll(ll2.m_lat * a + ll1.m_lat * (1 - a), ll2.m_lon * a + ll1.m_lon * (1 - a));

          // Get diff between approx altitude and real one.
          auto const alt = manager.GetAltitude(ll);
          if (alt == geometry::kInvalidAltitude)
          {
            LOG_SHORT(LWARNING, ("Invalid altitude for the middle point:", ll));
            ++added;
          }
          else
          {
            auto const approxAlt = static_cast<geometry::Altitude>(std::round(alt2 * a + alt1 * (1 - a)));
            if (abs(alt - approxAlt) >= std::max(1, abs(alt)/10))  // 10%
              ++added;
          }
        }
      }
    });

    auto const percent = added * 100.0 / all;
    std::string prefix = "Low";
    if (percent >= 1)
      prefix = "Huge";
    else if (added >= 1000)
      prefix = "Medium";

    LOG_SHORT(LINFO, (prefix, file.GetCountryName(), "all:", all, "invalid:", invalid, "added:", added, "%:", percent));
  });
}

int main(int argc, char * argv[])
{
  gflags::SetUsageMessage("SRTM coverage checker.");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_srtm_path.empty())
  {
    LOG_SHORT(LERROR, ("SRTM files directory is not specified."));
    return -1;
  }

  classificator::Load();

  if (!FLAGS_mwm_path.empty())
  {
    SafeTileManager manager(FLAGS_srtm_path);

    Platform & platform = GetPlatform();
    platform.SetWritableDirForTests(FLAGS_mwm_path);

    if (FLAGS_check_dist)
      CheckDistance(manager);
    else
      CheckCoverage(manager);
  }
  else
  {
    generator::SrtmTileManager manager(FLAGS_srtm_path);

    using namespace std;
    cout << "Enter lat lon. Or Ctrl + C to exit." << endl;

    while (true)
    {
      double lat, lon;
      cin >> lat >> lon;
      if (!cin)
      {
        cout << "Invalid lat lon." << endl;
        cin.clear();
        cin.ignore(10000, '\n');
      }
      else
      {
        auto const & tile = manager.GetTile({lat, lon});
        cout << "H = " << tile.GetHeight({lat, lon}) <<
                "; Trg = " << tile.GetTriangleHeight({lat, lon}) <<
                "; Bilinear = " << tile.GetBilinearHeight({lat, lon});
        cout << endl;
      }
    }
  }

  return 0;
}
