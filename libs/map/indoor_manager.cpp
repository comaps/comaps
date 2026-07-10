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

void IndoorManager::UpdateViewport(ScreenBase const & screen)
{
  m_currentModelView = screen;

  if (df::GetDrawTileScale(screen) < kMinIndoorZoom)
  {
    ++m_generation;
    if (!m_levels.empty())
    {
      m_levels.clear();
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
  return result;
}

std::string IndoorManager::GetActiveLevel() const
{
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

  m_backgroundRunner(
      [this, generation, rect]()
  {
    if (generation != m_generation)
      return;

    std::vector<double> levels;
    m_forEachFeature(rect, [&levels](FeatureType & ft)
    {
      feature::TypesHolder const types(ft);
      if (!ftypes::IsIndoorChecker::Instance()(types))
        return;

      auto parsed = indoor::ParseLevels(ft.GetMetadata(feature::Metadata::FMD_LEVEL));
      if (parsed.empty())
        parsed.push_back(0.0);  // Indoor feature without a level is on the ground floor.

      for (double const level : parsed)
      {
        if (std::none_of(levels.begin(), levels.end(),
                         [level](double existing) { return LevelsEqual(existing, level); }))
          levels.push_back(level);
      }
    }, scales::GetUpperScale());

    std::sort(levels.begin(), levels.end());

    m_uiRunner([this, generation, levels = std::move(levels)]() mutable
    { ApplyScanResult(generation, std::move(levels)); });
  });
}

void IndoorManager::ApplyScanResult(uint64_t generation, std::vector<double> && levels)
{
  if (generation != m_generation)
    return;

  if (levels == m_levels)
    return;

  m_levels = std::move(levels);

  if (m_levels.empty())
  {
    NotifyListener();
    return;
  }

  bool const activePresent = std::any_of(m_levels.begin(), m_levels.end(),
                                         [this](double level) { return LevelsEqual(level, m_activeLevel); });
  if (!activePresent)
  {
    // Prefer the ground floor, otherwise the lowest available level.
    bool const hasGround =
        std::any_of(m_levels.begin(), m_levels.end(), [](double level) { return LevelsEqual(level, 0.0); });
    SetActiveLevel(hasGround ? 0.0 : m_levels.front(), true /* notifyDrape */);
  }

  NotifyListener();
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
