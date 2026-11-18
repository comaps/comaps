#include "map/traffic_manager.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/traffic_generator.hpp"
#include "drape_frontend/visual_params.hpp"

#include "geometry/mercator.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

using namespace std::chrono;

namespace
{
/** Poll interval for traffic data */
auto constexpr kUpdateInterval = minutes(1);

auto constexpr kOutdatedDataTimeout = minutes(5) + kUpdateInterval;
auto constexpr kNetworkErrorTimeout = minutes(20);

auto constexpr kMaxRetriesCount = 5;
}  // namespace

TrafficManager::TrafficManager(GetMwmsByRectFn const & getMwmsByRectFn, size_t maxCacheSizeBytes,
                               traffic::TrafficObserver & observer)
  : m_getMwmsByRectFn(getMwmsByRectFn)
  , m_currentDataVersion(0)
  , m_state(TrafficState::Disabled)
  , m_isRunning(true)
  , m_isPaused(false)
  , m_thread(&TrafficManager::ThreadRoutine, this)
{
  CHECK(m_getMwmsByRectFn != nullptr, ());
}

TrafficManager::~TrafficManager()
{
#ifdef DEBUG
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    ASSERT(!m_isRunning, ());
  }
#endif
}

void TrafficManager::Teardown()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isRunning)
      return;
    m_isRunning = false;
  }
  m_condition.notify_one();
  m_thread.join();
}

TrafficManager::TrafficState TrafficManager::GetState() const
{
  return m_state;
}

void TrafficManager::SetStateListener(TrafficStateChangedFn const & onStateChangedFn)
{
  m_onStateChangedFn = onStateChangedFn;
}

void TrafficManager::SetEnabled(bool enabled)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (enabled == IsEnabled())
      return;
    ChangeState(enabled ? TrafficState::Enabled : TrafficState::Disabled);
  }

  m_drapeEngine.SafeCall(&df::DrapeEngine::EnableTraffic, enabled);

  if (enabled)
  {
  }
  else
  {
  }
}

void TrafficManager::Clear()
{
}

void TrafficManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
}

void TrafficManager::SetCurrentDataVersion(int64_t dataVersion)
{
  m_currentDataVersion = dataVersion;
}

void TrafficManager::OnMwmDeregistered(platform::LocalCountryFile const & countryFile)
{
  // TODO we don’t need this any more (called by Framework::OnMapDeregistered())
}

void TrafficManager::OnDestroySurface()
{
  Pause();
}

void TrafficManager::OnRecoverSurface()
{
  Resume();
}

void TrafficManager::UpdateActiveMwms(m2::RectD const & rect, std::vector<MwmSet::MwmId> & lastMwmsByRect,
                                      std::set<MwmSet::MwmId> & activeMwms)
{
  auto mwms = m_getMwmsByRectFn(rect);
  if (lastMwmsByRect == mwms)
    return;
  lastMwmsByRect = mwms;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    activeMwms.clear();
    for (auto const & mwm : mwms)
      if (mwm.IsAlive())
        activeMwms.insert(mwm);
  }
}

void TrafficManager::UpdateMyPosition(MyPosition const & myPosition)
{
  // Side of square around |myPosition|. Every mwm which is covered by the square
  // will get traffic info.
  double constexpr kSquareSideM = 5000.0;
  m_currentPosition = {myPosition, true /* initialized */};

  if (!IsEnabled() || IsInvalidState() || m_isPaused)
    return;

  m2::RectD const rect = mercator::RectByCenterXYAndSizeInMeters(myPosition.m_position, kSquareSideM / 2.0);
  // Request traffic.
}

void TrafficManager::UpdateViewport(ScreenBase const & screen)
{
  m_currentModelView = {screen, true /* initialized */};

  if (!IsEnabled() || IsInvalidState() || m_isPaused)
    return;

  if (df::GetZoomLevel(screen.GetScale()) < df::kRoadClass0ZoomLevel)
    return;

  // Request traffic.
  UpdateActiveMwms(screen.ClipRect(), m_lastDrapeMwmsByRect, m_activeDrapeMwms);
}

void TrafficManager::ThreadRoutine()
{
}

bool TrafficManager::IsEnabled() const
{
  return m_state != TrafficState::Disabled;
}

bool TrafficManager::IsInvalidState() const
{
  return m_state == TrafficState::NetworkError;
}

void TrafficManager::ChangeState(TrafficState newState)
{
  if (m_state == newState)
    return;

  m_state = newState;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, newState]()
  {
    if (m_onStateChangedFn != nullptr)
      m_onStateChangedFn(newState);
  });
}

void TrafficManager::OnEnterForeground()
{
  Resume();
}

void TrafficManager::OnEnterBackground()
{
  Pause();
}

void TrafficManager::Pause()
{
  m_isPaused = true;
}

void TrafficManager::Resume()
{
  if (!m_isPaused)
    return;

  m_isPaused = false;
}

void TrafficManager::SetSimplifiedColorScheme(bool simplified)
{
  m_hasSimplifiedColorScheme = simplified;
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetSimplifiedTrafficColors, simplified);
}

std::string DebugPrint(TrafficManager::TrafficState state)
{
  switch (state)
  {
  case TrafficManager::TrafficState::Disabled: return "Disabled";
  case TrafficManager::TrafficState::Enabled: return "Enabled";
  case TrafficManager::TrafficState::WaitingData: return "WaitingData";
  case TrafficManager::TrafficState::Outdated: return "Outdated";
  case TrafficManager::TrafficState::NoData: return "NoData";
  case TrafficManager::TrafficState::NetworkError: return "NetworkError";
  case TrafficManager::TrafficState::ExpiredData: return "ExpiredData";
  case TrafficManager::TrafficState::ExpiredApp: return "ExpiredApp";
  default: ASSERT(false, ("Unknown state"));
  }
  return "Unknown";
}
