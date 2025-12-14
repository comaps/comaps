#include "generator/panoramax_collector.hpp"

#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <cstdint>
#include <fstream>
#include <sstream>

namespace generator
{

PanoramaxCollector::PanoramaxCollector() = default;

bool PanoramaxCollector::LoadImageryData(std::string const & coordsFilePath, std::string const & /* tilesDir */)
{
  LOG(LINFO, ("Loading Panoramax imagery from:", coordsFilePath));

  std::ifstream file(coordsFilePath, std::ios::binary);
  if (!file.is_open())
  {
    LOG(LWARNING, ("Failed to open Panoramax coords file:", coordsFilePath));
    return false;
  }

  // Read header
  uint32_t version;
  uint64_t count;

  file.read(reinterpret_cast<char*>(&version), sizeof(version));
  file.read(reinterpret_cast<char*>(&count), sizeof(count));

  if (version != 1)
  {
    LOG(LWARNING, ("Unsupported Panoramax coords file version:", version));
    return false;
  }

  LOG(LINFO, ("Loading", count, "Panoramax imagery points..."));

  // Read coordinates and build spatial index
  std::vector<ImageryPoint> points;
  points.reserve(count);

  for (uint64_t i = 0; i < count; ++i)
  {
    double lat, lon;
    file.read(reinterpret_cast<char*>(&lat), sizeof(lat));
    file.read(reinterpret_cast<char*>(&lon), sizeof(lon));

    // Convert to Mercator coordinates
    m2::PointD mercPt = mercator::FromLatLon(lat, lon);
    points.emplace_back(mercPt);

    if ((i + 1) % 500000 == 0)
      LOG(LINFO, ("Loaded", i + 1, "/", count, "points"));
  }

  if (!file)
  {
    LOG(LWARNING, ("Error reading Panoramax coords file"));
    return false;
  }

  file.close();

  LOG(LINFO, ("Building spatial index for", points.size(), "points..."));

  // Build spatial index
  for (auto const & pt : points)
    m_imageryTree.Add(pt);

  m_stats.m_totalImageryPoints = static_cast<uint32_t>(points.size());

  LOG(LINFO, ("Panoramax data loaded:", m_stats.m_totalImageryPoints, "imagery points"));
  return true;
}

bool PanoramaxCollector::EnrichRoad(feature::FeatureBuilder & fb)
{
  // Only process roads
  static auto const & isRoad = ftypes::IsWayChecker::Instance();
  if (!isRoad(fb.GetTypes()))
    return false;

  m_stats.m_totalRoads++;

  // Check if road already has Panoramax flag
  if (fb.GetMetadata().Has(feature::Metadata::FMD_PANORAMAX))
    return false;

  // Get road geometry
  std::vector<m2::PointD> roadPoints;
  fb.ForEachPoint([&roadPoints](m2::PointD const & pt) {
    roadPoints.push_back(pt);
  });

  if (roadPoints.empty())
    return false;

  // Calculate road bounding box
  m2::RectD roadBBox;
  for (auto const & pt : roadPoints)
    roadBBox.Add(pt);

  // Expand bounding box by threshold distance in Mercator units
  auto const center = roadBBox.Center();
  auto const inflateRect = mercator::RectByCenterXYAndSizeInMeters(center, kDistanceThresholdM);
  roadBBox.Inflate(inflateRect.SizeX() / 2.0, inflateRect.SizeY() / 2.0);

  // Query imagery tree for nearby points
  bool hasImagery = false;

  m_imageryTree.ForEachInRect(roadBBox, [&](ImageryPoint const & imgPt) {
    if (hasImagery)
      return;  // Already found, early exit

    // Check distance to each road point
    for (auto const & roadPt : roadPoints)
    {
      double const distM = mercator::DistanceOnEarth(roadPt, imgPt.m_point);
      if (distM <= kDistanceThresholdM)
      {
        hasImagery = true;
        return;
      }
    }
  });

  if (hasImagery)
  {
    fb.GetMetadata().Set(feature::Metadata::FMD_PANORAMAX, "yes");
    m_stats.m_roadsWithImagery++;
    return true;
  }

  return false;
}

std::string DebugPrint(PanoramaxCollector::Stats const & s)
{
  std::ostringstream out;
  out << "PanoramaxCollector::Stats {"
      << " totalRoads: " << s.m_totalRoads
      << ", roadsWithImagery: " << s.m_roadsWithImagery
      << " (" << (s.m_totalRoads > 0 ? (s.m_roadsWithImagery * 100.0 / s.m_totalRoads) : 0) << "%)"
      << ", totalImageryPoints: " << s.m_totalImageryPoints
      << " }";
  return out.str();
}

}  // namespace generator
