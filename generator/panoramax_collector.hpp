#pragma once

#include "generator/feature_builder.hpp"

#include "geometry/point2d.hpp"
#include "geometry/tree4d.hpp"

#include "base/geo_object_id.hpp"

#include <string>
#include <vector>

namespace generator
{
/// Enriches road features with Panoramax street-level imagery availability flag.
/// Loads Panoramax imagery coordinates from tile index and marks roads within threshold distance.
class PanoramaxCollector
{
public:
  // Distance threshold for matching roads to imagery (meters)
  static double constexpr kDistanceThresholdM = 20.0;

  /// ImageryPoint represents a single point where street-level imagery exists
  struct ImageryPoint
  {
    m2::PointD m_point;  // Mercator coordinates

    ImageryPoint() = default;
    explicit ImageryPoint(m2::PointD const & pt) : m_point(pt) {}

    m2::RectD GetLimitRect() const { return m2::RectD(m_point, m_point); }
  };

  /// Statistics for debugging/logging
  struct Stats
  {
    uint32_t m_totalRoads = 0;
    uint32_t m_roadsWithImagery = 0;
    uint32_t m_totalImageryPoints = 0;
    uint32_t m_tilesProcessed = 0;

    friend std::string DebugPrint(Stats const & s);
  };

  PanoramaxCollector();

  /// Load Panoramax imagery data from tile directory
  /// @param tileIndexPath Path to tile_index.json
  /// @param tilesDir Directory containing .mvt tiles
  /// @return true if data loaded successfully
  bool LoadImageryData(std::string const & tileIndexPath, std::string const & tilesDir);

  /// Check if a road has nearby imagery and enrich it
  /// @param fb FeatureBuilder for a road segment
  /// @return true if imagery was found within threshold
  bool EnrichRoad(feature::FeatureBuilder & fb);

  Stats const & GetStats() const { return m_stats; }

private:
  /// Spatial index of imagery points
  m4::Tree<ImageryPoint> m_imageryTree;

  /// Statistics
  Stats m_stats;

  /// Decode an MVT tile and extract imagery coordinates
  std::vector<m2::PointD> DecodeMVTTile(std::string const & tilePath,
                                         int tileX, int tileY, int zoom);
};

}  // namespace generator
