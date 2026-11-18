#pragma once

#include "traffic/traffic_info.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/pointers.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/thread.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace df
{
class DrapeEngine;
}  // namespace df

class TrafficManager final
{
public:
  /**
   * @brief Global state of traffic information.
   */
  enum class TrafficState
  {
    /** Traffic is disabled, no traffic data will be retrieved or considered for routing. */
    Disabled,
    /** Traffic is enabled and working normally (the first request may not have been scheduled yet). */
    Enabled,
    /** At least one request is currently pending. */
    WaitingData,
    /** At least one MWM has stale traffic data. */
    Outdated,
    /** Traffic data for at least one MWM was invalid or not found on the server. */
    NoData,
    /** At least one request failed or timed out. */
    NetworkError,
    /** Traffic data could not be retrieved because the map data is outdated. */
    ExpiredData,
    /** Traffic data could not be retrieved because the app version is outdated. */
    ExpiredApp
  };

  struct MyPosition
  {
    m2::PointD m_position = m2::PointD(0.0, 0.0);
    bool m_knownPosition = false;

    MyPosition() = default;
    MyPosition(m2::PointD const & position) : m_position(position), m_knownPosition(true) {}
  };

  using TrafficStateChangedFn = std::function<void(TrafficState)>;
  using GetMwmsByRectFn = std::function<std::vector<MwmSet::MwmId>(m2::RectD const &)>;

  TrafficManager(GetMwmsByRectFn const & getMwmsByRectFn, size_t maxCacheSizeBytes,
                 traffic::TrafficObserver & observer);
  ~TrafficManager();

  void Teardown();

  TrafficState GetState() const;
  void SetStateListener(TrafficStateChangedFn const & onStateChangedFn);

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  /**
   * @brief Sets the version of the MWM used locally.
   */
  void SetCurrentDataVersion(int64_t dataVersion);

  void SetEnabled(bool enabled);
  bool IsEnabled() const;

  void UpdateViewport(ScreenBase const & screen);
  void UpdateMyPosition(MyPosition const & myPosition);


  void OnDestroySurface();
  void OnRecoverSurface();
  void OnMwmDeregistered(platform::LocalCountryFile const & countryFile);

  void OnEnterForeground();
  void OnEnterBackground();

  void SetSimplifiedColorScheme(bool simplified);
  bool HasSimplifiedColorScheme() const { return m_hasSimplifiedColorScheme; }

private:





  void ThreadRoutine();


  void UpdateActiveMwms(m2::RectD const & rect, std::vector<MwmSet::MwmId> & lastMwmsByRect,
                        std::set<MwmSet::MwmId> & activeMwms);

  // This is a group of methods that haven't their own synchronization inside.

  void Clear();
  void ChangeState(TrafficState newState);

  bool IsInvalidState() const;

  void UniteActiveMwms(std::set<MwmSet::MwmId> & activeMwms) const;

  void Pause();
  void Resume();

  template <class F>
  void ForEachActiveMwm(F && f) const
  {
    std::set<MwmSet::MwmId> activeMwms;
    UniteActiveMwms(activeMwms);
    std::for_each(activeMwms.begin(), activeMwms.end(), std::forward<F>(f));
  }

  GetMwmsByRectFn m_getMwmsByRectFn;

  df::DrapeEngineSafePtr m_drapeEngine;
  std::atomic<int64_t> m_currentDataVersion;

  // These fields have a flag of their initialization.
  std::pair<MyPosition, bool> m_currentPosition = {MyPosition(), false};
  std::pair<ScreenBase, bool> m_currentModelView = {ScreenBase(), false};

  std::atomic<TrafficState> m_state;
  TrafficStateChangedFn m_onStateChangedFn;

  bool m_hasSimplifiedColorScheme = true;


  bool m_isRunning;
  std::condition_variable m_condition;

  std::vector<MwmSet::MwmId> m_lastDrapeMwmsByRect;
  std::set<MwmSet::MwmId> m_activeDrapeMwms;
  std::set<MwmSet::MwmId> m_activeRoutingMwms;


  std::atomic<bool> m_isPaused;

  std::mutex m_mutex;
  threads::SimpleThread m_thread;
};

extern std::string DebugPrint(TrafficManager::TrafficState state);
