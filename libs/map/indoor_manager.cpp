#include "map/indoor_manager.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "platform/platform.hpp"

#include <algorithm>
#include <cmath>

namespace
{
// Floor closest to ground. Ties resolve upward, so {-1,1} picks 1.
double ClosestToGround(std::vector<double> const & levels)
{
  return *std::min_element(levels.begin(), levels.end(), [](double lhs, double rhs)
  {
    double const dl = std::fabs(lhs), dr = std::fabs(rhs);
    return dl != dr ? dl < dr : lhs > rhs;
  });
}
}  // namespace

IndoorManager::IndoorManager(ForEachFeatureFn forEachFeature, TaskRunnerFn backgroundRunner, TaskRunnerFn uiRunner)
  : m_forEachFeature(std::move(forEachFeature))
  , m_backgroundRunner(std::move(backgroundRunner))
  , m_uiRunner(std::move(uiRunner))
{
  if (!m_backgroundRunner)
    m_backgroundRunner = [](std::function<void()> && task)
    { GetPlatform().RunTask(Platform::Thread::File, std::move(task)); };
  if (!m_uiRunner)
    m_uiRunner = [](std::function<void()> && task) { GetPlatform().RunTask(Platform::Thread::Gui, std::move(task)); };

  m_alive = std::make_shared<IndoorManager *>(this);
}

IndoorManager::~IndoorManager()
{
  // Any scan still queued or in flight sees a null and returns without touching this object.
  *m_alive = nullptr;
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

  if (m_suspended || df::GetDrawTileScale(screen) < indoor::kMinZoom)
  {
    ++m_generation;
    m_pendingScanCenter.reset();
    if (m_complex)
      ApplyScanResult(std::nullopt);
    return;
  }

  ScheduleScan(screen.GetOrg());
}

void IndoorManager::SetSuspended(bool suspended)
{
  if (m_suspended == suspended)
    return;

  m_suspended = suspended;
  if (m_currentModelView)
    UpdateViewport(*m_currentModelView);
}

void IndoorManager::Invalidate()
{
  if (m_currentModelView)
    UpdateViewport(*m_currentModelView);
}

std::vector<double> IndoorManager::GetViewportLevels() const
{
  std::vector<double> result;
  if (!m_complex)
    return result;

  result.reserve(m_complex->m_levels.size());
  // Topmost floor first, the way a level selector reads.
  for (auto it = m_complex->m_levels.rbegin(); it != m_complex->m_levels.rend(); ++it)
    result.push_back(*it);
  return result;
}

bool IndoorManager::SelectLevel(double level)
{
  if (!m_complex)
    return false;

  level = indoor::Normalize(level);
  auto const & levels = m_complex->m_levels;
  if (std::find(levels.begin(), levels.end(), level) == levels.end())
    return false;

  if (indoor::LevelsEqual(m_activeLevel, level))
    return true;

  m_activeLevel = level;
  PushToDrape();
  NotifyListener();
  return true;
}

void IndoorManager::ScheduleScan(m2::PointD const & center)
{
  // One scan at a time. During a gesture the newest center simply replaces the pending one.
  if (m_scanInFlight)
  {
    m_pendingScanCenter = center;
    return;
  }

  m_scanInFlight = true;
  RunScan(center);
}

void IndoorManager::RunScan(m2::PointD const & center)
{
  uint64_t const generation = ++m_generation;

  // Snapshot for hysteresis, which the scan only reads.
  auto const current = m_complex;

  m_backgroundRunner([alive = m_alive, generation, center, current]()
  {
    auto * const self = *alive;
    if (self == nullptr)
      return;

    std::optional<indoor::Complex> complex;
    if (generation == self->m_generation)
    {
      auto const source = [self](m2::RectD const & rect, indoor::FeatureFn const & fn)
      {
        self->m_forEachFeature(rect, [&fn](FeatureType & ft)
        {
          feature::TypesHolder const types(ft);
          auto view = indoor::MakeFeatureView(ft, types, scales::GetUpperScale(), true /* withGeometry */);
          if (view.m_isBuilding || view.m_isIndoor || view.m_isLeveled)
            fn(view);
        }, scales::GetUpperScale());
      };
      complex = indoor::ScanForActiveComplex(center, source, current.get());
    }

    // Always posted, so the in-flight slot is released even when the result is stale.
    self->m_uiRunner([alive, generation, complex = std::move(complex)]() mutable
    {
      auto * const me = *alive;
      if (me == nullptr)
        return;

      me->m_scanInFlight = false;

      if (generation == me->m_generation)
        me->ApplyScanResult(std::move(complex));

      if (me->m_pendingScanCenter)
      {
        m2::PointD const next = *me->m_pendingScanCenter;
        me->m_pendingScanCenter.reset();
        me->ScheduleScan(next);
      }
    });
  });
}

void IndoorManager::ApplyScanResult(std::optional<indoor::Complex> && complex)
{
  bool const wasActive = m_complex != nullptr;
  auto const previousId = wasActive ? m_complex->m_id : FeatureID();
  auto const previousLevels = wasActive ? m_complex->m_levels : std::vector<double>();
  auto const previousRect = wasActive ? m_complex->m_rect : m2::RectD();
  size_t const previousTriangles = wasActive ? m_complex->m_triangles.size() : 0;

  m_complex = complex ? std::make_shared<indoor::Complex const>(std::move(*complex)) : nullptr;

  if (!m_complex)
  {
    if (!wasActive)
      return;
    PushToDrape();
    NotifyListener();
    return;
  }

  // Keep the chosen floor while the same building stays in focus.
  bool const sameComplex = wasActive && previousId == m_complex->m_id;
  auto const & levels = m_complex->m_levels;
  if (!sameComplex || std::find(levels.begin(), levels.end(), m_activeLevel) == levels.end())
    m_activeLevel = ClosestToGround(levels);

  // The footprint too, not just the floors, since absorbing grows it without changing id or levels.
  bool const sameShape =
      previousRect == m_complex->m_rect && previousTriangles == m_complex->m_triangles.size();
  if (sameComplex && sameShape && previousLevels == levels)
    return;

  PushToDrape();
  NotifyListener();
}

void IndoorManager::PushToDrape()
{
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetIndoor, GetActive());
}

void IndoorManager::NotifyListener()
{
  if (m_onLevelsChangedFn)
    m_onLevelsChangedFn(GetViewportLevels(), m_activeLevel);
}
