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

// Tracks the indoor complex under the viewport center.
//
// Every public method and all private methods run on the GUI thread (uiRunner)
// except RunScan, which runs on the File thread (backgroundRunner).
// We write to Drape by-value via SetIndoor, no further complexity there.
class IndoorManager final
{
public:
  using ForEachFeatureFn =
      std::function<void(m2::RectD const &, std::function<void(FeatureType &)> const &, int scale)>;
  using TaskRunnerFn = std::function<void(std::function<void()> &&)>;
  // Holds a listener callback to get an updated list of floors. Empty means no indoor data and the level selector should hide.
  using LevelsChangedFn = std::function<void(std::vector<double> const & levels, double activeLevel)>;

  /// Scans for any indoor building complexes under the viewport center.
  /// @param forEachFeature callback to read features within a rect
  /// @param backgroundRunner background scan task (only set inside tests)
  /// @param uiRunner UI result task (only set inside tests)
  explicit IndoorManager(ForEachFeatureFn forEachFeature, TaskRunnerFn backgroundRunner = {},
                         TaskRunnerFn uiRunner = {});

  ~IndoorManager();

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetLevelsListener(LevelsChangedFn const & fn);

  void UpdateViewport(ScreenBase const & screen);
  void Invalidate();
  // Set true to drop any active complex and refuses to scan until unsuspended.
  void SetSuspended(bool suspended);

  bool IsActive() const { return m_complex != nullptr; }
  std::vector<double> GetViewportLevels() const;
  // Returns false when the level isn't a floor of the active complex.
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
  // Scans run on a thread outliving this object, so they hold a handle instead of "this."
  std::shared_ptr<IndoorManager *> m_alive;
  bool m_suspended = false;
  bool m_scanInFlight = false;
  std::optional<m2::PointD> m_pendingScanCenter;

  std::shared_ptr<indoor::Complex const> m_complex;
  double m_activeLevel = 0.0;

  std::shared_ptr<indoor::Complex const> m_lastKnownComplex;
};
