#pragma once

#include "geometry/latlon.hpp"

#include "indexer/feature_altitude.hpp"

#include "geometry/point_with_altitude.hpp"

#include "base/macros.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace generator
{
class SrtmTile
{
public:
  SrtmTile() : m_valid(false) {}
  SrtmTile(SrtmTile && rhs);

  void Init(std::string const & dir, ms::LatLon const & coord);

  inline bool IsValid() const { return m_valid; }
  /// @return Height in meters at |coord| or kInvalidAltitude.
  /// @{
  /// Nearest serialized height.
  geometry::Altitude GetHeight(ms::LatLon const & coord) const;
  /// Height from underlying triangle (better than GetHeight).
  geometry::Altitude GetTriangleHeight(ms::LatLon const & coord) const;
  /// @}

  static std::string GetBase(ms::LatLon const & coord);
  static ms::LatLon GetCenter(ms::LatLon const & coord);
  static std::string GetPath(std::string const & dir, std::string const & base);

  /// Used in unit tests only to prepare mock tile.
  geometry::Altitude * DataForTests(size_t & sz);

private:
  static ms::LatLon GetCoordInSeconds(ms::LatLon const & coord);
  geometry::Altitude GetHeightRC(size_t row, size_t col) const;

  inline geometry::Altitude const * Data() const
  {
    return reinterpret_cast<geometry::Altitude const *>(m_data.data());
  }

  inline size_t Size() const { return m_data.size() / sizeof(geometry::Altitude); }
  void Invalidate();

  std::vector<uint8_t> m_data;
  bool m_valid;

  DISALLOW_COPY(SrtmTile);
};

class SrtmTileManager
{
public:
  explicit SrtmTileManager(std::string const & dir) : m_dir(dir) {}

  SrtmTile const & GetTile(ms::LatLon const & coord);

  geometry::Altitude GetHeight(ms::LatLon const & coord)
  {
    return GetTile(coord).GetHeight(coord);
  }

  geometry::Altitude GetTriangleHeight(ms::LatLon const & coord)
  {
    return GetTile(coord).GetTriangleHeight(coord);
  }

  void Purge();

private:
  using LatLonKey = std::pair<int32_t, int32_t>;
  static LatLonKey GetKey(ms::LatLon const & coord);

  std::string m_dir;

  struct Hash
  {
    size_t operator()(LatLonKey const & key) const
    {
      return (static_cast<size_t>(key.first) << 32u) | static_cast<size_t>(key.second);
    }
  };

  using MapT = std::unordered_map<LatLonKey, SrtmTile, Hash>;
  MapT m_tiles;

  DISALLOW_COPY(SrtmTileManager);
};
}  // namespace generator
