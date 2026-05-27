#pragma once

#include "drape_frontend/batcher_bucket.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/matrix.hpp"

#include <string>

namespace df
{
// TileKey identifies a single map tile and the version of its geometry.
//
// The (x, y, zoomLevel) triplet is the standard slippy-map tile coordinate.
// Zoom 1 = world overview; zoom 17 = street level (max data zoom is clamped
// by ClipTileZoomByMaxDataZoom in tile_utils.hpp).
//
// Generation counters
// -------------------
// m_generation increments every time the tile's base geometry needs to be
// rebuilt (e.g. map style changed, data updated). FrontendRenderer holds a
// m_maxGeneration and discards any RenderGroup whose generation is older.
// BackendRenderer's ReadManager tracks the current generation to skip reads
// that are already superseded before they finish.
//
// m_userMarksGeneration is a separate counter that increments only when
// user marks (bookmarks, search results, route waypoints) change. This lets
// user-mark geometry be refreshed independently of base map geometry.
//
// operator< / operator== intentionally ignore both generation fields so that
// tile coordinate lookups (std::map, std::set) work across geometry versions.
// Use LessStrict / EqualStrict when you need to distinguish generations —
// BatchersPool uses TileKeyStrictComparator for exactly this reason.
struct TileKey
{
  TileKey();
  TileKey(int x, int y, uint8_t zoomLevel);
  TileKey(TileKey const & key, uint64_t generation, uint64_t userMarksGeneration);

  // Operators < and == do not consider parameter m_generation.
  // m_generation is used to determine a generation of geometry for this tile key.
  // Geometry with different generations must be able to group by (x, y, zoomlevel).
  bool operator<(TileKey const & other) const;
  bool operator==(TileKey const & other) const;

  // These methods implement strict comparison of tile keys. It's necessary to merger of
  // batches which must not merge batches with different m_generation.
  bool LessStrict(TileKey const & other) const;
  bool EqualStrict(TileKey const & other) const;

  m2::RectD GetGlobalRect(bool clipByDataMaxZoom = true) const;

  math::Matrix<float, 4, 4> GetTileBasedModelView(ScreenBase const & screen) const;

  m2::PointI GetTileCoords() const;

  uint64_t GetHashValue(BatcherBucket bucket) const;

  std::string Coord2String() const;

  int m_x;
  int m_y;
  uint8_t m_zoomLevel;

  uint64_t m_generation;
  uint64_t m_userMarksGeneration;
};

struct TileKeyStrictComparator
{
  bool operator()(TileKey const & lhs, TileKey const & rhs) const { return lhs.LessStrict(rhs); }
};

std::string DebugPrint(TileKey const & key);
}  // namespace df
