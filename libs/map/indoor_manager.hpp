#pragma once

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
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
// which is used by drape to filter indoor features. Modeled on IsolinesManager.
class IndoorManager final
{
public:
  // |levels| are formatted level labels sorted from the topmost floor down; empty means no indoor
  // data in the viewport (the level selector UI should be hidden).
  using LevelsChangedFn = std::function<void(std::vector<std::string> const & levels, std::string const & activeLevel)>;
  // Called (on the Gui thread) after each viewport scan when debug mode is on.
  // isIndoorTyped=true for indoor=* geometry (triggers indoor mode),
  // false for level=* POIs/footways (floor-filtered by drape but don't trigger mode).
  struct DebugRect
  {
    m2::RectD rect;
    double level;
    bool isIndoorTyped;
    std::string name;   // feature display name (may be empty for unnamed features)
    std::string types;  // space-separated classificator type strings, e.g. "shop-mall"
  };
  using DebugRectsChangedFn = std::function<void(std::vector<DebugRect> const &)>;
  using ForEachFeatureFn =
      std::function<void(m2::RectD const &, std::function<void(FeatureType &)> const &, int scale)>;
  using TaskRunnerFn = std::function<void(std::function<void()> &&)>;
  // Returns whether indoor mode may be entered or switched to a different building right now. When
  // it returns false, new indoor data in the viewport is ignored so nothing pops up (e.g. while
  // driving during navigation).
  using CanEnterFn = std::function<bool()>;
  // Returns whether an already-active indoor context should be preserved against viewport-driven
  // deactivation (zoom-out, panning off the building). True e.g. throughout route planning and
  // navigation, so fitting the route view doesn't drop the level chooser/filtering.
  using ShouldHoldFn = std::function<bool()>;

  explicit IndoorManager(ForEachFeatureFn && forEachFeature);

  // Replaces the default platform thread runners (used by tests to run synchronously).
  void SetTaskRunners(TaskRunnerFn const & backgroundRunner, TaskRunnerFn const & uiRunner);

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetLevelsListener(LevelsChangedFn const & fn);
  // Called (on the Gui thread) whenever indoor mode toggles on/off, i.e. the viewport gains or
  // loses indoor data. Used e.g. to temporarily disable 3D buildings while indoors.
  void SetModeChangedListener(std::function<void()> const & fn);
  // Sets a predicate deciding whether indoor mode may be entered or switched (see CanEnterFn).
  void SetCanEnterPredicate(CanEnterFn const & fn);
  // Sets a predicate deciding whether an already-active indoor context is held against viewport-
  // driven deactivation (see ShouldHoldFn).
  void SetShouldHoldPredicate(ShouldHoldFn const & fn);
  // Enables/disables indoor debug mode. When enabled, DebugRectsChangedFn is called after each
  // scan with the bounding rect + level of every indoor feature found in the viewport.
  void SetDebugEnabled(bool enabled);
  bool IsDebugEnabled() const { return m_debugEnabled; }
  void SetDebugRectsListener(DebugRectsChangedFn const & fn);

  // Returns a human-readable description of the debug feature nearest to |mercatorPt|, or empty
  // if no debug rects are stored (debug mode off or no scan yet).
  std::string GetDebugFeatureAt(m2::PointD const & mercatorPt) const;

  // Returns a human-readable description of the feature that last triggered indoor mode activation,
  // or empty if indoor mode hasn't been entered yet.
  std::string GetActivatingInfo() const { return m_lastActivatingInfo; }

  void UpdateViewport(ScreenBase const & screen);
  void Invalidate();

  // True while the viewport has indoor data and level filtering is active. Gui thread only.
  bool IsActive() const { return !m_levels.empty(); }

  std::vector<std::string> GetViewportLevels() const;
  std::string GetActiveLevel() const;
  void SelectLevel(std::string const & level);

private:
  void ScheduleScan(m2::RectD const & rect);
  void ApplyScanResult(uint64_t generation, std::vector<double> && levels,
                        std::vector<m2::RectD> && polygonRects,
                        std::vector<DebugRect> && debugRects, std::string && triggerInfo);
  void SetActiveLevel(double level, bool notifyDrape);
  void NotifyListener();
  void NotifyModeChanged();
  // Preserve an already-active context against viewport-driven deactivation (routing in progress).
  bool ShouldHold() const;
  // May we enter/switch indoor context now? Defaults to true when no predicate is set.
  bool CanEnter() const;

  ForEachFeatureFn m_forEachFeature;
  TaskRunnerFn m_backgroundRunner;
  TaskRunnerFn m_uiRunner;

  LevelsChangedFn m_onLevelsChangedFn;
  DebugRectsChangedFn m_onDebugRectsChangedFn;
  std::function<void()> m_onModeChangedFn;
  CanEnterFn m_canEnterIndoorFn;
  ShouldHoldFn m_shouldHoldIndoorFn;
  std::atomic<bool> m_debugEnabled{false};

  df::DrapeEngineSafePtr m_drapeEngine;

  std::optional<ScreenBase> m_currentModelView;

  std::atomic<uint64_t> m_generation{0};

  // Distinct levels present in the viewport, sorted ascending. Gui thread only.
  std::vector<double> m_levels;
  std::vector<m2::RectD> m_indoorPolygonRects;
  double m_activeLevel = 0.0;

  // Last set of debug rects from the most recent scan; guarded by mutex so GetDebugFeatureAt
  // can be called from any thread (e.g. JNI).
  mutable std::mutex m_debugRectsMutex;
  std::vector<DebugRect> m_lastDebugRects;

  // Description of the feature that most recently triggered indoor mode activation (Gui thread only).
  std::string m_lastActivatingInfo;
};
