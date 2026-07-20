#include "map/indoor_manager.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/indoor_level.hpp"
#include "indexer/scales.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <cmath>

namespace
{
int constexpr kMinIndoorZoom = 17;
double constexpr kLevelEpsilon = 1e-9;

bool LevelsEqual(double lhs, double rhs)
{
  return std::fabs(lhs - rhs) < kLevelEpsilon;
}
}  // namespace

bool IndoorManager::ShouldHold() const
{
  return m_shouldHoldIndoorFn && m_shouldHoldIndoorFn();
}

bool IndoorManager::CanEnter() const
{
  return !m_canEnterIndoorFn || m_canEnterIndoorFn();
}

IndoorManager::IndoorManager(ForEachFeatureFn && forEachFeature) : m_forEachFeature(std::move(forEachFeature))
{
  m_backgroundRunner = [](std::function<void()> && task)
  { GetPlatform().RunTask(Platform::Thread::File, std::move(task)); };
  m_uiRunner = [](std::function<void()> && task) { GetPlatform().RunTask(Platform::Thread::Gui, std::move(task)); };
}

void IndoorManager::SetTaskRunners(TaskRunnerFn const & backgroundRunner, TaskRunnerFn const & uiRunner)
{
  m_backgroundRunner = backgroundRunner;
  m_uiRunner = uiRunner;
}

void IndoorManager::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
}

void IndoorManager::SetLevelsListener(LevelsChangedFn const & fn)
{
  m_onLevelsChangedFn = fn;
}

void IndoorManager::SetModeChangedListener(std::function<void()> const & fn)
{
  m_onModeChangedFn = fn;
}

void IndoorManager::SetCanEnterPredicate(CanEnterFn const & fn)
{
  m_canEnterIndoorFn = fn;
}

void IndoorManager::SetShouldHoldPredicate(ShouldHoldFn const & fn)
{
  m_shouldHoldIndoorFn = fn;
}

void IndoorManager::SetDebugEnabled(bool enabled)
{
  m_debugEnabled = enabled;
  if (!enabled && m_onDebugRectsChangedFn)
    m_onDebugRectsChangedFn({});
  Invalidate();
}

void IndoorManager::SetDebugRectsListener(DebugRectsChangedFn const & fn)
{
  m_onDebugRectsChangedFn = fn;
}

void IndoorManager::UpdateViewport(ScreenBase const & screen)
{
  m_currentModelView = screen;

  LOG(LINFO, ("IndoorManager::UpdateViewport, zoom =", df::GetDrawTileScale(screen)));
  if (df::GetDrawTileScale(screen) < kMinIndoorZoom)
  {
    ++m_generation;
    // While holding (route planning/navigation), keep an active context so fitting the route view
    // doesn't drop the chooser/filtering. Otherwise fully deactivate: drape stops level-filtering,
    // 3D buildings come back, and the chooser hides.
    if (!m_levels.empty() && !ShouldHold())
    {
      m_levels.clear();
      m_drapeEngine.SafeCall(&df::DrapeEngine::SetIndoorLevel, indoor::kNoActiveLevel);
      NotifyModeChanged();
      NotifyListener();
    }
    return;
  }

  ScheduleScan(screen.ClipRect());
}

void IndoorManager::Invalidate()
{
  if (m_currentModelView)
    UpdateViewport(*m_currentModelView);
}

std::vector<std::string> IndoorManager::GetViewportLevels() const
{
  std::vector<std::string> result;
  result.reserve(m_levels.size());
  // Topmost floor first, as in a real level selector.
  for (auto it = m_levels.rbegin(); it != m_levels.rend(); ++it)
    result.push_back(indoor::FormatLevel(*it));
  LOG(LINFO, ("IndoorManager ViewportLevels: ", m_levels.size()));
  return result;
}

std::string IndoorManager::GetActiveLevel() const
{
  LOG(LINFO, ("IndoorManager ActiveLevel: ", m_activeLevel));
  return indoor::FormatLevel(m_activeLevel);
}

void IndoorManager::SelectLevel(std::string const & level)
{
  auto const parsed = indoor::ParseLevels(level);
  if (parsed.size() != 1)
    return;

  SetActiveLevel(parsed.front(), true /* notifyDrape */);
  NotifyListener();
}

void IndoorManager::ScheduleScan(m2::RectD const & rect)
{
  uint64_t const generation = ++m_generation;

  bool const debugEnabled = m_debugEnabled;
  m_backgroundRunner(
      [this, generation, rect, debugEnabled]()
  {
    if (generation != m_generation)
      return;

    std::vector<double> levels;
    std::vector<IndoorManager::DebugRect> debugRects;
    size_t totalCount = 0;
    m_forEachFeature(rect, [&levels, &debugRects, &totalCount, debugEnabled](FeatureType & ft)
    {
      ++totalCount;
      feature::TypesHolder const types(ft);
      bool const isIndoor = ftypes::IsIndoorChecker::Instance()(types);

      // In debug mode also collect level-tagged non-indoor features (the same set drape
      // floor-filters via ShouldSkipIndoorFeature's "isLeveled" path).
      std::string_view const levelMeta = ft.GetMetadata(feature::Metadata::FMD_LEVEL);
      bool const isLeveled = !levelMeta.empty()
          && !ftypes::IsBuildingChecker::Instance()(types)
          && !ftypes::IsBuildingPartChecker::Instance()(types);

      if (!isIndoor && !(debugEnabled && isLeveled))
        return;

      auto parsed = indoor::ParseLevels(levelMeta);
      if (isIndoor && parsed.empty())
        parsed.push_back(0.0);  // Indoor feature without a level is on the ground floor.

      for (double const level : parsed)
      {
        if (isIndoor && std::none_of(levels.begin(), levels.end(),
                         [level](double existing) { return LevelsEqual(existing, level); }))
          levels.push_back(level);
        if (debugEnabled)
          debugRects.emplace_back(ft.GetLimitRect(scales::GetUpperScale()), level, isIndoor);
      }
    }, scales::GetUpperScale());

    std::sort(levels.begin(), levels.end());
    LOG(LINFO, ("IndoorManager scan finished, rect =", rect, "total =", totalCount, "levels count =", levels.size()));

    m_uiRunner([this, generation, levels = std::move(levels), debugRects = std::move(debugRects)]() mutable
    { ApplyScanResult(generation, std::move(levels), std::move(debugRects)); });
  });
}

void IndoorManager::ApplyScanResult(uint64_t generation, std::vector<double> && levels,
                                     std::vector<IndoorManager::DebugRect> && debugRects)
{
  LOG(LINFO, ("IndoorManager::ApplyScanResult, gen =", generation, "cur =", m_generation.load(), "levels =", levels.size()));
  if (generation != m_generation)
    return;

  bool const active = !m_levels.empty();

  if (!active)
  {
    // Gate entering indoor mode: if entry is currently disallowed (e.g. driving during navigation),
    // drop any indoor data found so nothing pops up.
    if (!levels.empty() && !CanEnter())
      levels.clear();
  }
  else
  {
    if (levels.empty())
    {
      // An active context lost its indoor data (panned off the building). While holding (route
      // planning/navigation) keep it frozen instead of deactivating.
      if (ShouldHold())
        return;
    }
    else if (levels != m_levels && !CanEnter())
    {
      // A different building came into view but switching isn't allowed now (e.g. driving past
      // other buildings): keep the current context frozen rather than popping to the new one.
      return;
    }
  }

  if (levels == m_levels)
    return;

  m_levels = std::move(levels);
  bool const isActive = !m_levels.empty();

  if (!isActive)
  {
    // No indoor data in the viewport: deactivate level filtering in drape so ordinary level-tagged
    // POIs stay visible. m_activeLevel is kept as the remembered floor for when we re-enter indoors.
    m_drapeEngine.SafeCall(&df::DrapeEngine::SetIndoorLevel, indoor::kNoActiveLevel);
    if (active)
      NotifyModeChanged();
    NotifyListener();
    if (m_debugEnabled && m_onDebugRectsChangedFn)
      m_onDebugRectsChangedFn(debugRects);
    return;
  }

  bool const activePresent = std::any_of(m_levels.begin(), m_levels.end(),
                                         [this](double level) { return LevelsEqual(level, m_activeLevel); });
  if (!activePresent)
  {
    // Prefer the ground floor, otherwise the lowest available level.
    bool const hasGround =
        std::any_of(m_levels.begin(), m_levels.end(), [](double level) { return LevelsEqual(level, 0.0); });
    m_activeLevel = hasGround ? 0.0 : m_levels.front();
  }

  // Always (re)assert the active level to drape: we may be re-entering an indoor context after being
  // empty (drape currently inactive) even when the remembered m_activeLevel is unchanged.
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetIndoorLevel, m_activeLevel);

  if (!active)
    NotifyModeChanged();
  NotifyListener();
  if (m_debugEnabled && m_onDebugRectsChangedFn)
    m_onDebugRectsChangedFn(debugRects);
}

void IndoorManager::SetActiveLevel(double level, bool notifyDrape)
{
  if (LevelsEqual(m_activeLevel, level))
    return;

  m_activeLevel = level;
  if (notifyDrape)
    m_drapeEngine.SafeCall(&df::DrapeEngine::SetIndoorLevel, level);
}

void IndoorManager::NotifyListener()
{
  if (m_onLevelsChangedFn)
    m_onLevelsChangedFn(GetViewportLevels(), GetActiveLevel());
}

void IndoorManager::NotifyModeChanged()
{
  if (m_onModeChangedFn)
    m_onModeChangedFn();
}
