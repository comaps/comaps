#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/triangle2d.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class FeatureType;

// The indoor domain model, in indexer because both libs/map and libs/drape_frontend must agree on it.
namespace indoor
{
// Snapping grid that keeps OSM half floors but collapses elevation noise like "-1.3;-1.7".
double constexpr kLevelStep = 0.5;
// Rejects absurd level=* values before they reach llround or the UI.
double constexpr kMaxLevel = 1000.0;
// Only show indoor mode at high zoom levels.
int constexpr kMinZoom = 17;
// Faster than this is considered driving, and we don't want to enter indoor mode at that speed.
double constexpr kMaxFollowSpeedMps = 3.0;  // ~11 km/h
// Hysteresis, so speed must fall this far back before indoor mode returns (prevent flickering)
double constexpr kResumeFollowSpeedMps = 2.0;  // ~7 km/h
// Older than this is considered no longer a relevant speed, e.g. after losing a GPS fix inside a building.
double constexpr kSpeedStaleSeconds = 10.0;
// How far away from viewport center a building can sit and still be considered active.
double constexpr kActivationBufferMeters = 20.0;
// How far out to look for the rest of a complex, so a giant train station still counts as one complex.
double constexpr kSearchMeters = 500.0;
// A polygon this large is a mistagged region, not a building.
double constexpr kMaxBuildingRectDeg = 0.1;
// The fraction of a building's footprint that rooms must cover to activate indoor mode (5%)
double constexpr kMinIndoorCoverage = 0.05;
// The smallest indoor area worth activating on, roughly 20 m square. Below this a building is considered noise.
double constexpr kMinComplexAreaM2 = 400.0;
// Minimum floors we'll activate an indoor level picker for (also reduces unnecessarily noisy indoor tagging)
size_t constexpr kMinLevels = 2;

// Snaps to the nearest kLevelStep. Every level in this namespace is normalized.
double Normalize(double level);

inline bool LevelsEqual(double lhs, double rhs)
{
  return Normalize(lhs) == Normalize(rhs);
}

/**
 * Parses level=* into sorted distinct normalized levels, e.g. "0", "1.5", "0;1;2", "0-2", "-2--1".
 * @return empty when any part of the value does not parse
 */
std::vector<double> ParseLevels(std::string_view s);

// True if s lists level. An unparsable or absent value matches nothing.
bool LevelsContain(std::string_view s, double level);

// Canonical label with a period separator, so it round-trips through ParseLevels. Not for display.
std::string FormatLevel(double level);

// One description of a feature, so the scan and the renderer are always in agreement.
struct FeatureView
{
  FeatureID m_id;
  // A building or building:part. Doesn't create floors by itself.
  bool m_isBuilding = false;
  // A building:part, which is always inside a building rather than standalone.
  bool m_isPart = false;
  bool m_isArea = false;
  bool m_isIndoor = false;
  // A transit platform. With a level=* it is part of the inside, even beyond the building above it.
  bool m_isPlatform = false;
  bool m_isLeveled = false;
  // layer=*. A building above 0 is stacked over the ground, i.e. a roof rather than an outline.
  int8_t m_layer = 0;
  m2::RectD m_rect;
  std::string m_level;
  // Filled only when the caller asked for geometry.
  std::vector<m2::TriangleD> m_triangles;

  // Whether this feature names a floor, so a complex may count it and the renderer may hide it.
  bool NamesAFloor() const { return m_isIndoor || (m_isLeveled && !m_isBuilding); }
};

/**
 * Turn a map feature into a unified FeatureView to use across both scan and renderer
 * @param withGeometry true only when we need triangle geometry to compute overlaps (during scan), false for speed
 */
FeatureView MakeFeatureView(FeatureType & ft, feature::TypesHolder const & types, int scale, bool withGeometry);

// The building complex on-screen. Call its methods to check how a feature should be handled.
struct Complex
{
  FeatureID m_id;
  m2::RectD m_rect;
  std::vector<m2::TriangleD> m_triangles;
  std::vector<double> m_levels;  // sorted, distinct, normalized
  // The building outlines this complex is made of. Anything else built over it is considered clutter.
  std::vector<FeatureID> m_members;

  // Check before hiding a building, since the complex's own outlines have to stay on screen.
  bool IsMember(FeatureID const & id) const;
  // Check whether a point falls inside the footprint, e.g. the map center or a corner of a shape.
  bool Contains(m2::PointD const & pt) const;
  // Check before absorbing a shape, since one already wholly inside would add nothing.
  bool ContainsPolygon(std::vector<m2::TriangleD> const & triangles) const;
  // Check whether a shape shares ground, which a bounding box cannot tell you reliably.
  bool OverlapsPolygon(std::vector<m2::TriangleD> const & triangles) const;
  // Check which building a feature belongs to, so a neighbor's rooms are not mistaken for ours.
  bool Owns(m2::RectD const & rect) const;
  // Check whether a feature is drawn over us at all, which is a looser test than Owns.
  bool Reaches(m2::RectD const & rect) const;
};

// The complex and current floor on-screen as one value. Its methods tell you how to draw each feature.
struct Active
{
  std::shared_ptr<Complex const> m_complex;
  double m_level = 0.0;

  // Check first. Everything below answers sensibly with indoor mode off, but this is cheaper.
  bool IsOn() const { return m_complex != nullptr; }

  // Check before drawing anything. True for other floors, other buildings, and clutter above us.
  bool Hides(FeatureView const & f) const;

  // Check before drawing a building in 3D, since its volume over us would hide the floor entirely.
  bool Flattens(std::vector<m2::TriangleD> const & triangles) const;

  // Check before drawing a transit platform, which needs to be rendered as a floor rather than in the background (the default.)
  bool DrawsAsFloor(FeatureView const & f) const;

  // Check before applying layer=*, because the chosen indoor floor already decides what should cover what.
  bool IgnoresLayer(FeatureView const & f) const;

  // Check regarding streets and other ground detail, which might otherwise cover subways etc.
  bool Sinks(FeatureView const & f) const;
};
}  // namespace indoor
