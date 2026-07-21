#include "map/indoor_manager.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "geometry/mercator.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/indoor_level.hpp"
#include "indexer/scales.hpp"

#include "i18n/localisation.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>
#include <sstream>

namespace
{
int constexpr kMinIndoorZoom = 17;
double constexpr kLevelEpsilon = 1e-9;
// City-scale polygons / transit networks sometimes tagged indoor=* would trigger indoor mode across
// a wide area and produce a giant debug box. Skip any feature whose bounding rect exceeds this.
// The world's largest indoor spaces (airport terminals, mega-malls) are well under 0.1°.
double constexpr kMaxIndoorRectDeg = 0.1;
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
    std::vector<m2::RectD> polygonRects;
    std::vector<IndoorManager::DebugRect> debugRects;
    std::string triggerInfo;
    size_t totalCount = 0;
    m_forEachFeature(rect, [&levels, &polygonRects, &debugRects, &triggerInfo, &totalCount, debugEnabled, rect](FeatureType & ft)
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

      // Compute limit rect once: needed for viewport intersection check, size-cap, and debug overlay.
      m2::RectD limitRect;
      bool isOversized = false;
      if (isIndoor || debugEnabled)
      {
        limitRect = ft.GetLimitRect(scales::GetUpperScale());

        // m_forEachFeature uses tile-level indexing and may return features from the same tile that
        // don't actually intersect the viewport. Filter them out here.
        if (!rect.IsIntersect(limitRect))
          return;

        isOversized = isIndoor &&
            (limitRect.SizeX() > kMaxIndoorRectDeg || limitRect.SizeY() > kMaxIndoorRectDeg);
      }
      // Only polygon (Area) indoor features trigger indoor mode; door nodes and wall ways don't.
      bool const isArea = isIndoor && ft.GetGeomType() == feature::GeomType::Area;

      // Collect human-readable info for logging and for the tap-to-identify feature.
      std::string featureName;
      std::string featureTypes;
      if (debugEnabled || (isIndoor && !isOversized))
      {
        auto const sv = ft.GetName(localisation::kDefaultNameIndex);
        if (!sv.empty())
          featureName = std::string(sv);

        std::ostringstream typeStream;
        bool first = true;
        for (uint32_t t : types)
        {
          if (!first) typeStream << ' ';
          typeStream << classif().GetFullObjectName(t);
          first = false;
        }
        featureTypes = typeStream.str();
      }

      for (double const level : parsed)
      {
        // Only non-oversized Area indoor features contribute levels and trigger indoor mode.
        if (isArea && !isOversized && std::none_of(levels.begin(), levels.end(),
                         [level](double existing) { return LevelsEqual(existing, level); }))
        {
          auto const ll = mercator::ToLatLon(limitRect.Center());
          LOG(LINFO, ("IndoorManager: indoor mode triggered by \"", featureName, "\" [", featureTypes,
                      "] level", level, "center lat/lon", ll.m_lat, ll.m_lon));
          if (triggerInfo.empty())
          {
            std::ostringstream o;
            if (!featureName.empty()) o << '"' << featureName << "\" ";
            o << "[" << featureTypes << "] (" << ll.m_lat << ", " << ll.m_lon << ")";
            triggerInfo = o.str();
          }
          levels.push_back(level);
        }
        if (debugEnabled)
          // Oversized indoor features appear faded (isIndoorTyped=false) to distinguish them.
          debugRects.push_back({limitRect, level, isIndoor && !isOversized, featureName, featureTypes});
      }
      // Collect bounding rect for proximity filtering (once per qualifying indoor polygon).
      if (isArea && !isOversized)
        polygonRects.push_back(limitRect);
    }, scales::GetUpperScale());

    std::sort(levels.begin(), levels.end());
    LOG(LINFO, ("IndoorManager scan finished, rect =", rect, "total =", totalCount, "levels count =", levels.size()));

    m_uiRunner([this, generation, levels = std::move(levels), polygonRects = std::move(polygonRects),
                debugRects = std::move(debugRects), triggerInfo = std::move(triggerInfo)]() mutable
    { ApplyScanResult(generation, std::move(levels), std::move(polygonRects), std::move(debugRects), std::move(triggerInfo)); });
  });
}

void IndoorManager::ApplyScanResult(uint64_t generation, std::vector<double> && levels,
                                     std::vector<m2::RectD> && polygonRects,
                                     std::vector<IndoorManager::DebugRect> && debugRects,
                                     std::string && triggerInfo)
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
  m_indoorPolygonRects = std::move(polygonRects);
  bool const isActive = !m_levels.empty();

  if (!isActive)
  {
    // No indoor data in the viewport: deactivate level filtering in drape so ordinary level-tagged
    // POIs stay visible. m_activeLevel is kept as the remembered floor for when we re-enter indoors.
    m_drapeEngine.SafeCall(&df::DrapeEngine::SetIndoorLevel, indoor::kNoActiveLevel, m_levels, m_indoorPolygonRects);
    if (active)
      NotifyModeChanged();
    NotifyListener();
    if (m_debugEnabled)
    {
      {
        std::lock_guard<std::mutex> lock(m_debugRectsMutex);
        m_lastDebugRects = debugRects;
      }
      if (m_onDebugRectsChangedFn)
        m_onDebugRectsChangedFn(debugRects);
    }
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
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetIndoorLevel, m_activeLevel, m_levels, m_indoorPolygonRects);

  if (!active)
  {
    m_lastActivatingInfo = std::move(triggerInfo);
    NotifyModeChanged();
  }
  NotifyListener();
  if (m_debugEnabled)
  {
    {
      std::lock_guard<std::mutex> lock(m_debugRectsMutex);
      m_lastDebugRects = debugRects;
    }
    if (m_onDebugRectsChangedFn)
      m_onDebugRectsChangedFn(debugRects);
  }
}

std::string IndoorManager::GetDebugFeatureAt(m2::PointD const & mercatorPt) const
{
  std::lock_guard<std::mutex> lock(m_debugRectsMutex);
  if (m_lastDebugRects.empty())
    return {};

  DebugRect const * best = nullptr;
  double bestDistSq = std::numeric_limits<double>::max();

  for (auto const & dr : m_lastDebugRects)
  {
    // Expand point features to a small area so they're tappable.
    m2::RectD r = dr.rect;
    double constexpr kMinHalf = 0.0005;  // ~55m; generous tap target for node features
    if (r.SizeX() < kMinHalf || r.SizeY() < kMinHalf)
      r = m2::RectD(r.Center() - m2::PointD(kMinHalf, kMinHalf),
                    r.Center() + m2::PointD(kMinHalf, kMinHalf));

    double const distSq = mercatorPt.SquaredLength(r.Center());
    if (distSq < bestDistSq)
    {
      bestDistSq = distSq;
      best = &dr;
    }
  }

  // Only report if within a reasonable tap radius (~110m in mercator degrees).
  double constexpr kMaxTapRadius = 0.001;
  if (!best || bestDistSq > kMaxTapRadius * kMaxTapRadius)
    return {};

  std::ostringstream out;
  if (!best->name.empty())
    out << '"' << best->name << "\" ";
  out << '[' << best->types << "] level:" << indoor::FormatLevel(best->level);
  auto const ll = mercator::ToLatLon(best->rect.Center());
  out << " (" << ll.m_lat << ',' << ll.m_lon << ')';
  return out.str();
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
