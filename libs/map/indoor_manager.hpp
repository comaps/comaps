#pragma once

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class FeatureType;

namespace df
{
class DrapeEngine;
}  // namespace df

// Tracks indoor (level=*) data in the current viewport and holds the active indoor level
// which is used by drape to filter indoor features. Based on IsolinesManager.
class IndoorManager final
{
public:
  // |levels| are formatted level labels sorted from the topmost floor down; empty means no indoor
  // data in the viewport (the level selector UI should be hidden).
  using LevelsChangedFn = std::function<void(std::vector<std::string> const & levels, std::string const & activeLevel)>;
  using ForEachFeatureFn =
      std::function<void(m2::RectD const &, std::function<void(FeatureType &)> const &, int scale)>;
  using TaskRunnerFn = std::function<void(std::function<void()> &&)>;
  // Returns whether indoor mode may be entered or switched to a different building right now. When
  // it returns false, new indoor data in the viewport is ignored so nothing pops up (i.e. while
  // driving during navigation).
  using CanEnterFn = std::function<bool()>;
  // Returns whether an already-active indoor context should be preserved against viewport-driven
  // deactivation (zoom-out, panning off the building). True throughout route planning and
  // navigation, so automatic panning and zooming doesn't exit indoor mode
  using ShouldHoldFn = std::function<bool()>;

  explicit IndoorManager(ForEachFeatureFn && forEachFeature);

  // Replaces the default platform thread runners (used by tests to run synchronously).
  void SetTaskRunners(TaskRunnerFn const & backgroundRunner, TaskRunnerFn const & uiRunner);

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetLevelsListener(LevelsChangedFn const & fn);
  // Called (on the GUI thread) whenever indoor mode toggles on/off, i.e. the viewport gains or
  // loses indoor data. Used to temporarily disable 3D buildings while indoors.
  void SetModeChangedListener(std::function<void()> const & fn);
  // Sets a predicate deciding whether indoor mode may be entered or switched (see CanEnterFn).
  void SetCanEnterPredicate(CanEnterFn const & fn);
  // Sets a predicate deciding whether an already-active indoor context is prevented from viewport-based deactivation (see ShouldHoldFn).
  void SetShouldHoldPredicate(ShouldHoldFn const & fn);

  void UpdateViewport(ScreenBase const & screen);
  void Invalidate();

  // True while the viewport has indoor data and level filtering is active. GUI thread only.
  bool IsActive() const { return !m_levels.empty(); }

  std::vector<std::string> GetViewportLevels() const;
  std::string GetActiveLevel() const;

  // Set the active level to the specified level, if valid. Otherwise do nothing.
  void SelectLevel(std::string const & level);

  // True if |position| is still within/near the currently active indoor context's polygon rects.
  // Used by the ShouldHold predicate so an active context is preserved only while the user is
  // actually near the building, not for an entire navigation session regardless of distance.
  bool IsNearActiveIndoorContext(m2::PointD const & position) const;

private:
  // Coalescing entry point: during a continuous zoom/pan gesture, UpdateViewport may call this on
  // every frame. Rather than queuing a background scan per call, only one scan is ever in flight;
  // additional requests just replace the pending rect and are picked up when the current scan
  // finishes (see RunScan).
  void ScheduleScan(m2::RectD const & rect);
  void RunScan(m2::RectD const & rect);
  void ApplyScanResult(uint64_t generation, std::vector<double> && levels,
                       std::vector<m2::RectD> && polygonRects);
  void SetActiveLevel(double level);
  void NotifyListener();
  void NotifyModeChanged();
  // Prevent an already-active context from being deactivated based on panning and zooming (during route planning mode).
  bool ShouldHold() const;
  // May we enter/switch indoor context now? Defaults to true when no predicate is set.
  bool CanEnter() const;

  ForEachFeatureFn m_forEachFeature;
  TaskRunnerFn m_backgroundRunner;
  TaskRunnerFn m_uiRunner;

  LevelsChangedFn m_onLevelsChangedFn;
  std::function<void()> m_onModeChangedFn;
  CanEnterFn m_canEnterIndoorFn;
  ShouldHoldFn m_shouldHoldIndoorFn;

  df::DrapeEngineSafePtr m_drapeEngine;

  std::optional<ScreenBase> m_currentModelView;

  std::atomic<uint64_t> m_generation{0};

  // Scan coalescing state. GUI thread only.
  bool m_scanInFlight = false;
  std::optional<m2::RectD> m_pendingScanRect;

  // Distinct levels present in the viewport, sorted ascending. GUI thread only.
  std::vector<double> m_levels;
  // Indoor polygon bounding rects, pre-expanded by kIndoorProximityMeters (see .cpp). Used both
  // locally by IsNearActiveIndoorContext and passed to drape for its own proximity filtering, so
  // the (latitude-aware) expansion only needs to happen once per scan rather than per feature.
  std::vector<m2::RectD> m_indoorPolygonRects;
  double m_activeLevel = 0.0;
};
