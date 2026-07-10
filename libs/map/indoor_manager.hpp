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
  using ForEachFeatureFn =
      std::function<void(m2::RectD const &, std::function<void(FeatureType &)> const &, int scale)>;
  using TaskRunnerFn = std::function<void(std::function<void()> &&)>;

  explicit IndoorManager(ForEachFeatureFn && forEachFeature);

  // Replaces the default platform thread runners (used by tests to run synchronously).
  void SetTaskRunners(TaskRunnerFn const & backgroundRunner, TaskRunnerFn const & uiRunner);

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetLevelsListener(LevelsChangedFn const & fn);

  void UpdateViewport(ScreenBase const & screen);
  void Invalidate();

  std::vector<std::string> GetViewportLevels() const;
  std::string GetActiveLevel() const;
  void SelectLevel(std::string const & level);

private:
  void ScheduleScan(m2::RectD const & rect);
  void ApplyScanResult(uint64_t generation, std::vector<double> && levels);
  void SetActiveLevel(double level, bool notifyDrape);
  void NotifyListener();

  ForEachFeatureFn m_forEachFeature;
  TaskRunnerFn m_backgroundRunner;
  TaskRunnerFn m_uiRunner;

  LevelsChangedFn m_onLevelsChangedFn;

  df::DrapeEngineSafePtr m_drapeEngine;

  std::optional<ScreenBase> m_currentModelView;

  std::atomic<uint64_t> m_generation{0};

  // Distinct levels present in the viewport, sorted ascending. Gui thread only.
  std::vector<double> m_levels;
  double m_activeLevel = 0.0;
};
