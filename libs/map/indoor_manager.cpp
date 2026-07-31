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

#include "geometry/rect_intersect.hpp"
#include "geometry/triangle2d.hpp"

#include <algorithm>
#include <cmath>

namespace
{
int constexpr kMinIndoorZoom = 17;
double constexpr kLevelEpsilon = 1e-9;
// City-sized polygons or transit networks sometimes tagged indoor=* would trigger indoor mode across
// a wide area. Skip any feature whose bounding rect exceeds this. The world's largest indoor spaces
// (airport terminals, mega-malls) should be well under 0.1°.
double constexpr kMaxIndoorRectDeg = 0.1;
bool LevelsEqual(double lhs, double rhs)
{
  return std::fabs(lhs - rhs) < kLevelEpsilon;
}
// True if triangle |a,b,c| actually overlaps |rect| (not just their bounding boxes).
// Catches a vertex inside the rect, the rect fully inside the triangle, or an edge crossing.
bool TriangleIntersectsRect(m2::RectD const & rect, m2::PointD const & a, m2::PointD const & b,
                            m2::PointD const & c)
{
  if (rect.IsPointInside(a) || rect.IsPointInside(b) || rect.IsPointInside(c))
    return true;
  if (m2::IsPointInsideTriangle(rect.Center(), a, b, c))
    return true;
  // m2::Intersect clips the segment to the rect (mutating the endpoints) and reports overlap.
  m2::PointD e1 = a, e2 = b;
  if (m2::Intersect(rect, e1, e2))
    return true;
  e1 = b, e2 = c;
  if (m2::Intersect(rect, e1, e2))
    return true;
  e1 = c, e2 = a;
  if (m2::Intersect(rect, e1, e2))
    return true;
  return false;
}
// Find the level closest to ground (0). |levels| must be non-empty. A floor and its negative counterpart
// (e.g. -1 and 1) are equidistant; ties resolve to the upper floor.
double ClosestToGround(std::vector<double> const & levels)
{
  return *std::min_element(levels.begin(), levels.end(), [](double lhs, double rhs)
  {
    double const dl = std::fabs(lhs), dr = std::fabs(rhs);
    if (!LevelsEqual(dl, dr))
      return dl < dr;
    return lhs > rhs;
  });
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

void IndoorManager::UpdateViewport(ScreenBase const & screen)
{
  m_currentModelView = screen;

  if (df::GetDrawTileScale(screen) < kMinIndoorZoom)
  {
    ++m_generation;
    // While holding (during route planning/navigation), keep an active context so automatic panning/zooming
    // doesn't exit indoor mode. Otherwise fully deactivate indoor mode: drape stops level-filtering,
    // 3D buildings come back, and the level chooser hides.
    if (!m_levels.empty() && !ShouldHold())
    {
      m_levels.clear();
      m_drapeEngine.SafeCall(&df::DrapeEngine::SetIndoorLevel, indoor::kNoActiveLevel, m_levels, m_indoorPolygonRects);
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
    std::vector<m2::RectD> polygonRects;
    m_forEachFeature(rect, [&levels, &polygonRects, rect](FeatureType & ft)
    {
      feature::TypesHolder const types(ft);
      auto const levelMeta = ft.GetMetadata(feature::Metadata::FMD_LEVEL);
      // Public-transport platforms carrying an explicit level=* act as indoor elements too: at
      // multi-level stations a floor is often occupied by nothing but a platform, which should still
      // register as a selectable level. Surface platforms (no level tag) are left alone.
      bool const isLeveledPlatform = !levelMeta.empty() && ftypes::IsPlatformChecker::Instance()(types);
      if (!ftypes::IsIndoorChecker::Instance()(types) && !isLeveledPlatform)
        return;

      // Only polygon (Area) features trigger indoor mode; door nodes and wall/edge ways don't.
      if (ft.GetGeomType() != feature::GeomType::Area)
        return;

      m2::RectD const limitRect = ft.GetLimitRect(scales::GetUpperScale());

      // Cheap bbox reject: m_forEachFeature uses tile-level indexing and may return features whose
      // bbox doesn't even touch the viewport.
      if (!rect.IsIntersect(limitRect))
        return;

      // City-scale polygons mis-tagged indoor=* would trigger indoor mode across a wide area. Skip.
      if (limitRect.SizeX() > kMaxIndoorRectDeg || limitRect.SizeY() > kMaxIndoorRectDeg)
        return;

      // Precise geometry check: a rotated/diamond-shaped station (e.g. Berlin Hbf) has large empty
      // corners in its bbox, so panning over a blank corner would spuriously trigger indoor mode.
      // Require the actual triangulated area to overlap the viewport.
      bool geometryIntersects = false;
      ft.ForEachTriangle(
          [&rect, &geometryIntersects](m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
      {
        if (!geometryIntersects && TriangleIntersectsRect(rect, a, b, c))
          geometryIntersects = true;
      }, scales::GetUpperScale());
      if (!geometryIntersects)
        return;

      auto parsed = indoor::ParseLevels(levelMeta);
      if (parsed.empty())
        parsed.push_back(0.0);  // Indoor feature without a level is on the ground floor.

      for (double const level : parsed)
      {
        if (std::none_of(levels.begin(), levels.end(),
                         [level](double existing) { return LevelsEqual(existing, level); }))
          levels.push_back(level);
      }
      // Bounding rect for proximity filtering of level-tagged POIs.
      polygonRects.push_back(limitRect);
    }, scales::GetUpperScale());

    std::sort(levels.begin(), levels.end());

    m_uiRunner([this, generation, levels = std::move(levels), polygonRects = std::move(polygonRects)]() mutable
    { ApplyScanResult(generation, std::move(levels), std::move(polygonRects)); });
  });
}

void IndoorManager::ApplyScanResult(uint64_t generation, std::vector<double> && levels,
                                    std::vector<m2::RectD> && polygonRects)
{
  if (generation != m_generation)
    return;

  bool const active = !m_levels.empty();

  if (!active)
  {
    // If starting indoor mode is currently disallowed (e.g. driving during navigation),
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

  // An identical level set is treated as the same building, so we keep the existing polygon rects
  // instead of re-asserting to drape (which would re-tile on every scan and flicker). If two adjacent
  // buildings ever shared an identical level set the proximity rects could be briefly stale, which is
  // acceptable for a ~5 m POI proximity filter.
  if (levels == m_levels)
    return;

  m_levels = std::move(levels);
  m_indoorPolygonRects = std::move(polygonRects);
  bool const isActive = !m_levels.empty();

  if (!isActive)
  {
    // No indoor data in the viewport: deactivate level filtering in drape so ordinary level-tagged POIs stay visible.
    m_drapeEngine.SafeCall(&df::DrapeEngine::SetIndoorLevel, indoor::kNoActiveLevel, m_levels, m_indoorPolygonRects);
    if (active)
      NotifyModeChanged();
    NotifyListener();
    return;
  }

  // |active| was captured before m_levels was reassigned, so !active means we're entering indoor
  // mode now (the viewport just gained indoor data).
  bool const entering = !active;
  bool const activePresent = std::any_of(m_levels.begin(), m_levels.end(),
                                         [this](double level) { return LevelsEqual(level, m_activeLevel); });
  // On entering a building, auto-select the floor closest to ground level (0). Also fall back to it
  // when the remembered floor isn't present in the newly focused building.
  if (entering || !activePresent)
    m_activeLevel = ClosestToGround(m_levels);

  // Always (re)assert the active level to drape: we may be re-entering an indoor context after being
  // empty (drape currently inactive) even when the remembered m_activeLevel is unchanged.
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetIndoorLevel, m_activeLevel, m_levels, m_indoorPolygonRects);

  if (entering)
    NotifyModeChanged();
  NotifyListener();
}

void IndoorManager::SetActiveLevel(double level, bool notifyDrape)
{
  if (LevelsEqual(m_activeLevel, level))
    return;

  m_activeLevel = level;
  if (notifyDrape)
    m_drapeEngine.SafeCall(&df::DrapeEngine::SetIndoorLevel, level, m_levels, m_indoorPolygonRects);
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
