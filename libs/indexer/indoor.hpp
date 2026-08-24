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
// Below this zoom indoor mode never activates.
int constexpr kMinZoom = 17;
// Faster than this is driving past buildings rather than walking through them, so above jogging pace.
double constexpr kMaxFollowSpeedMps = 3.0;  // ~11 km/h
// Hysteresis, so speed must fall this far back before indoor mode returns and the scene stops churning.
double constexpr kResumeFollowSpeedMps = 2.0;  // ~7 km/h
// Older than this says nothing about now, e.g. after losing the fix on the way into a station.
double constexpr kSpeedStaleSeconds = 10.0;
// How far off the screen center a building can sit and still count as the one you are looking at.
double constexpr kActivationBufferMeters = 20.0;
// How far out to look for the rest of a complex, so a big station resolves from either end.
double constexpr kSearchMeters = 500.0;
// A polygon this large is a mistagged region, not a building.
double constexpr kMaxBuildingRectDeg = 0.1;
// Fraction of a building's footprint that rooms must cover before it counts as having an inside, 5%.
double constexpr kMinIndoorCoverage = 0.05;
// Smallest footprint worth entering, roughly 20 m square. Below this it is a closet or a kiosk.
double constexpr kMinComplexAreaM2 = 400.0;
// Minimum floors worth a picker, which also keeps stray one-off indoor tags out.
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

// One description of a feature, so the scan and the renderer cannot disagree about it.
struct FeatureView
{
  FeatureID m_id;
  // Carries a building or building:part type. Such a feature names no floor of its own.
  bool m_isBuilding = false;
  // A building:part, which is always inside a building rather than one in its own right.
  bool m_isPart = false;
  bool m_isArea = false;
  bool m_isIndoor = false;
  // A platform. With a level=* it is part of the inside, even beyond the building above it.
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
 * Call this to turn a map feature into the facts indoor mode needs, so the scan and the renderer
 * always read a feature the same way.
 * @param types passed in because both callers already have one
 * @param withGeometry false on the render path, where triangles cost more than every other field
 */
FeatureView MakeFeatureView(FeatureType & ft, feature::TypesHolder const & types, int scale, bool withGeometry);

// The building complex on screen. Ask it about a feature to decide how that feature is treated.
struct Complex
{
  FeatureID m_id;
  m2::RectD m_rect;
  std::vector<m2::TriangleD> m_triangles;
  std::vector<double> m_levels;  // sorted, distinct, normalized
  // The building outlines this complex is made of. Anything else built over it is clutter.
  std::vector<FeatureID> m_members;

  // Ask before hiding a building, since the complex's own outlines have to stay on screen.
  bool IsMember(FeatureID const & id) const;
  // Ask whether a point falls inside the footprint, e.g. the map center or a corner of a shape.
  bool Contains(m2::PointD const & pt) const;
  // Ask before absorbing a shape, since one already wholly inside would add nothing.
  bool ContainsPolygon(std::vector<m2::TriangleD> const & triangles) const;
  // Ask whether a shape shares ground, which a bounding box cannot tell you reliably.
  bool OverlapsPolygon(std::vector<m2::TriangleD> const & triangles) const;
  // Ask which building a feature belongs to, so a neighbor's rooms are not mistaken for ours.
  bool Owns(m2::RectD const & rect) const;
  // Ask whether a feature is drawn over us at all, which is a looser test than Owns.
  bool Reaches(m2::RectD const & rect) const;
};

// The complex and floor on screen as one value. Ask it how to draw each feature.
struct Active
{
  std::shared_ptr<Complex const> m_complex;
  double m_level = 0.0;

  // Check first. Everything below answers sensibly with indoor mode off, but this is cheaper.
  bool IsOn() const { return m_complex != nullptr; }

  // Ask before drawing anything. True for other floors, other buildings, and clutter above us.
  bool Hides(FeatureView const & f) const;

  // Ask before drawing a building in 3D, since its volume over us would hide the floor entirely.
  bool Flattens(std::vector<m2::TriangleD> const & triangles) const;

  // Ask before drawing a platform, which belongs in front as the floor rather than in the background.
  bool DrawsAsFloor(FeatureView const & f) const;

  // Ask before applying layer=*, because the chosen floor already decides what covers what.
  bool IgnoresLayer(FeatureView const & f) const;

  // Ask about streets and other ground detail, which must not be drawn across an upper or lower floor.
  bool Sinks(FeatureView const & f) const;
};
}  // namespace indoor
