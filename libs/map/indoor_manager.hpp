#pragma once

#include "map/indoor_scan.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"

#include <atomic>
#include <memory>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class FeatureType;

namespace df
{
class DrapeEngine;
}  // namespace df

// Tracks the complex under the viewport center. GUI thread only, except the scan itself.
class IndoorManager final
{
public:
  using ForEachFeatureFn =
      std::function<void(m2::RectD const &, std::function<void(FeatureType &)> const &, int scale)>;
  using TaskRunnerFn = std::function<void(std::function<void()> &&)>;
  // Floors top first, and empty means no indoor data so the picker should hide.
  using LevelsChangedFn = std::function<void(std::vector<double> const & levels, double activeLevel)>;

  // Runners default to the platform threads, and tests inject their own.
  explicit IndoorManager(ForEachFeatureFn forEachFeature, TaskRunnerFn backgroundRunner = {},
                         TaskRunnerFn uiRunner = {});

  ~IndoorManager();

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetLevelsListener(LevelsChangedFn const & fn);

  void UpdateViewport(ScreenBase const & screen);
  void Invalidate();
  // Drops any active complex and refuses to scan until unsuspended. The caller decides why.
  void SetSuspended(bool suspended);

  bool IsActive() const { return m_complex != nullptr; }
  std::vector<double> GetViewportLevels() const;
  // Ignored unless the level is a floor of the active complex.
  bool SelectLevel(double level);

  double GetActiveLevelValue() const { return m_activeLevel; }
  std::shared_ptr<indoor::Complex const> GetComplex() const { return m_complex; }
  indoor::Active GetActive() const { return {m_complex, m_activeLevel}; }

private:
  void ScheduleScan(m2::PointD const & center);
  void RunScan(m2::PointD const & center);
  void ApplyScanResult(std::optional<indoor::Complex> && complex);
  void PushToDrape();
  void NotifyListener();

  ForEachFeatureFn m_forEachFeature;
  TaskRunnerFn m_backgroundRunner;
  TaskRunnerFn m_uiRunner;
  LevelsChangedFn m_onLevelsChangedFn;

  df::DrapeEngineSafePtr m_drapeEngine;
  std::optional<ScreenBase> m_currentModelView;

  std::atomic<uint64_t> m_generation{0};
  // Scans run on a thread outliving this object, so they hold a handle instead of a raw this.
  std::shared_ptr<IndoorManager *> m_alive;
  bool m_suspended = false;
  bool m_scanInFlight = false;
  std::optional<m2::PointD> m_pendingScanCenter;

  std::shared_ptr<indoor::Complex const> m_complex;
  double m_activeLevel = 0.0;

  // A scan superseded by a later one before it finishes still gets a chance to run (only the
  // instant a new one *starts* is skipped, see RunScan), but its result is discarded once done:
  // by the time it lands, m_generation has moved on and applying it would show a building the
  // viewport already left. Read the discarded result geometrically instead of throwing it away:
  // keep it here regardless of generation, and let the next scan's own Contains/Reaches check
  // (in ScanForActiveComplex) decide whether it's still relevant. That's what lets a rapid zoom
  // gesture, which supersedes scans faster than any one of them can finish, keep hitting the
  // "same complex" fast path instead of redoing the expensive building search on every frame.
  std::shared_ptr<indoor::Complex const> m_lastKnownComplex;
};
