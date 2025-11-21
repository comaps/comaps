#include "map/traffic_manager.hpp"

#include "routing/routing_helpers.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "platform/platform.hpp"

#include "traffxml/traff_model_xml.hpp"

using namespace std::chrono;

namespace
{
/**
 * Poll interval for traffic data
 */
auto constexpr kUpdateInterval = minutes(1);

/**
 * Purge interval for expired traffic messages
 */
auto constexpr kPurgeInterval = minutes(1);

auto constexpr kOutdatedDataTimeout = minutes(5) + kUpdateInterval;
auto constexpr kNetworkErrorTimeout = minutes(20);

auto constexpr kMaxRetriesCount = 5;

/**
 * Interval at which the Drape engine gets traffic updates while messages are being processed.
 */
auto constexpr kDrapeUpdateInterval = seconds(10);

/**
 * Interval at which the traffic observer gets traffic updates while messages are being processed.
 */
auto constexpr kObserverUpdateInterval = minutes(1);

/**
 * Interval at which the message cache file is updated while messages are being processed.
 */
auto constexpr kStorageUpdateInterval = minutes(1);

/**
 * File name at which traffic data is persisted.
 */
auto constexpr kTrafficXMLFileName = "traffic.xml";

/**
 * Threshold by which viewport or user position can change without requiring the nessage queue to
 * be resorted.
 */
auto constexpr kPositionThreshold = 1000.0;
}  // namespace

TrafficManager::TrafficManager(DataSource & dataSource, CountryInfoGetterFn countryInfoGetter,
                               const CountryParentNameGetterFn &countryParentNameGetter,
                               GetMwmsByRectFn const & getMwmsByRectFn, size_t maxCacheSizeBytes,
                               routing::RoutingSession & routingSession)
  : m_dataSource(dataSource)
  , m_countryInfoGetterFn(countryInfoGetter)
  , m_countryParentNameGetterFn(countryParentNameGetter)
  , m_getMwmsByRectFn(getMwmsByRectFn)
  , m_routingSession(routingSession)
  , m_currentDataVersion(0)
  , m_state(TrafficState::Disabled)
  , m_isRunning(true)
  , m_isPaused(false)
  , m_thread(&TrafficManager::ThreadRoutine, this)
{
  CHECK(m_getMwmsByRectFn != nullptr, ());
  GetPlatform().RunTask(Platform::Thread::Gui, [this]() {
    m_routingSession.SetChangeSessionStateCallback([this](routing::SessionState previous, routing::SessionState current) {
      OnChangeRoutingSessionState(previous, current);
    });
  });
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

std::map<std::string, traffxml::TraffMessage> TrafficManager::GetMessageCache()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_messageCache;
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
  /*
   * Whether to notify interested parties that traffic data has been updated.
   * This is based on the return value of RestoreCache().
   */
  bool notifyUpdate = false;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (enabled == IsEnabled())
       return;
    if (enabled)
    {
      if (!m_traffDecoder)
        // deferred decoder initialization (requires maps to be loaded)
        m_traffDecoder = make_unique<traffxml::DefaultTraffDecoder>(m_dataSource, m_countryInfoGetterFn,
                                                                    m_countryParentNameGetterFn, m_messageCache);
      if (!m_storage && !IsTestMode())
      {
        m_storage = std::make_unique<traffxml::LocalStorage>(kTrafficXMLFileName);
        notifyUpdate = RestoreCache();
        m_lastStorageUpdate = steady_clock::now();
      }
    }
    ChangeState(enabled ? TrafficState::Enabled : TrafficState::Disabled);
  }

  m_drapeEngine.SafeCall(&df::DrapeEngine::EnableTraffic, enabled);

  if (enabled)
  {
    if (notifyUpdate)
      OnTrafficDataUpdate();
    else
      RecalculateSubscription(true);
    m_canSetMode = false;
  }
  else
  {
    Unsubscribe();
    m_routingSession.OnTrafficInfoClear();
  }
}

void TrafficManager::Clear()
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_messageCache.clear();
    m_feedQueue.clear();
  }
  OnTrafficDataUpdate();
}

void TrafficManager::SetTrafficUpdateCallbackFn(TrafficUpdateCallbackFn && fn)
{
  m_trafficUpdateCallbackFn = std::move(fn);
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

void TrafficManager::OnChangeRoutingSessionState(routing::SessionState previous, routing::SessionState current)
{
  // TODO assert we’re running on the UI thread
  LOG(LINFO, ("Routing session state changed from", previous, "to", current));
  LOG(LINFO, ("Running on thread", std::this_thread::get_id()));

  m_routingSessionState = current;

  /*
   * Filter based on session state (see routing_callbacks.hpp for states and transitions).
   *
   * GetAllRegions() seems to get fresh data during build (presumably also rebuild), which will be
   * available on the next state transition (to RouteNotStarted or OnRoute) and remain unchanged
   * until the next route (re)build. Therefore, calling GetAllRegions() when new state is one of
   * RouteNotStarted, OnRoute or RouteNoFollowing, and clearing MWMs when the new state is
   * NoValidRoute, and ignoring all other transitions, should work for our purposes.
   * There may be cases where we first calculate the route, then subscribe to the regions and only
   * then get notified about a traffic problem on the route, requiring us to recalculate.
   */
  std::set<std::string> mwmNames;
  if (current == routing::SessionState::RouteNotStarted
      || current == routing::SessionState::OnRoute
      || current == routing::SessionState::RouteNoFollowing)
    /*
     * GetAllRegions() may block when run for the first time. This should happen during routing, so
     * the call here would always return cached results without blocking, causing no problems on the
     * UI thread.
     *
     * Note that this method is called before the state is updated internally, thus `GetAllRegions()`
     * and any other functions would still have `previous` as their state.
     */
    m_routingSession.GetAllRegions(mwmNames);
  else if (current == routing::SessionState::NoValidRoute)
    mwmNames.clear();
  else
    // for all other states, keep current set
    return;

  LOG(LINFO, ("Router MWMs:", mwmNames));

  std::set<MwmSet::MwmId> mwms;
  for (auto const & mwmName : mwmNames)
  {
    MwmSet::MwmId mwmId = m_dataSource.GetMwmIdByCountryFile(platform::CountryFile(mwmName));
    if (mwmId.IsAlive())
      mwms.insert(mwmId);
  }

  LOG(LINFO, ("MWM set:", mwms));

  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (mwms != m_activeRoutingMwms)
    {
      m_activeMwmsChanged = true;
      std::swap(mwms, m_activeRoutingMwms);

      if ((m_activeDrapeMwms.empty() && m_activePositionMwms.empty() && m_activeRoutingMwms.empty())
          || !IsEnabled() || IsInvalidState() || IsPausedAndNotRouting())
        return;

      m_condition.notify_one();
    }
  }
}

void TrafficManager::RecalculateSubscription(bool forceRenewal)
{
  if (!IsEnabled() || IsPausedAndNotRouting())
    return;

  if (m_currentModelView.second)
    UpdateViewport(m_currentModelView.first);
  if (m_currentPosition.second)
    UpdateMyPosition(m_currentPosition.first);

  {
    std::lock_guard<std::mutex> lock(m_mutex);

    /*
     * If UpdateViewport() or UpdateMyPosition() had changes, they would also have updated the
     * routing MWMs and reset m_activeMwmsChanged. If neither of them had changes and
     * m_activeMwmsChanged is true, it indicates changes in route MWMs which we need to process.
     * If `forceRenewal` is true, we set `m_activeMwmsChanged` to true in order to force renewal of
     * all subscriptions.
     */
    m_activeMwmsChanged |= forceRenewal;
    if (m_activeMwmsChanged)
    {
      if ((m_activeDrapeMwms.empty() && m_activePositionMwms.empty() && m_activeRoutingMwms.empty())
          || IsInvalidState())
        return;

      m_condition.notify_one();
    }
  }
}

void TrafficManager::Invalidate(MwmSet::MwmId const & mwmId)
{
  auto const mwmRect = mwmId.GetInfo()->m_bordersRect; // m2::RectD
  traffxml::TraffFeed invalidated;

  for (auto it = m_messageCache.begin(); it != m_messageCache.end(); )
  {
    if (!it->second.m_location)
    {
      it++;
      continue;
    }

    bool isInvalid = false;

    // invalidate if decoded location uses a previous version of the MWM
    for (auto const & [decodedMwmId, coloring] : it->second.m_decoded)
      if (decodedMwmId.GetInfo()->GetCountryName() == mwmId.GetInfo()->GetCountryName())
        isInvalid = true;

    // invalidate if bounding rect of reference points intersects with bounding rect of MWM
    if (!isInvalid)
    {
      m2::RectD locationRect;
      for (auto const & point : {it->second.m_location.value().m_from,
           it->second.m_location.value().m_via,
           it->second.m_location.value().m_at,
           it->second.m_location.value().m_to})
        if (point)
          locationRect.Add(mercator::FromLatLon(point.value().m_coordinates));
      isInvalid = locationRect.IsIntersect(mwmRect);
    }

    if (isInvalid)
    {
      traffxml::TraffMessage message(it->second);
      message.m_decoded.clear();
      invalidated.push_back(message);
      it = m_messageCache.erase(it);
    }
    else
      ++it;
  }

  if (!invalidated.empty())
  {
    m_feedQueue.insert(m_feedQueue.begin(), invalidated);
    m_condition.notify_one();
  }
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
    m_activeMwmsChanged = true;
    activeMwms.clear();
    for (auto const & mwm : mwms)
      if (mwm.IsAlive())
        activeMwms.insert(mwm);

    if ((m_activeDrapeMwms.empty() && m_activePositionMwms.empty() && m_activeRoutingMwms.empty())
        || !IsEnabled() || IsInvalidState() || IsPausedAndNotRouting())
      return;

    m_condition.notify_one();
  }
}

void TrafficManager::UpdateMyPosition(MyPosition const & myPosition)
{
  // Side of square around |myPosition|. Every mwm which is covered by the square
  // will get traffic info.
  double constexpr kSquareSideM = 5000.0;
  m_currentPosition = {myPosition, true /* initialized */};

  if (!m_currentPositionLazy.second
      || (mercator::DistanceOnEarth(m_currentPositionLazy.first.m_position, myPosition.m_position) > kPositionThreshold))
  {
    m_isFeedQueueSortInvalid = true;
    m_currentPositionLazy = m_currentPosition;
  }

  if (!IsEnabled() || IsInvalidState() || IsPausedAndNotRouting())
    return;

  m2::RectD const rect = mercator::RectByCenterXYAndSizeInMeters(myPosition.m_position, kSquareSideM / 2.0);
  // Request traffic.
  UpdateActiveMwms(rect, m_lastPositionMwmsByRect, m_activePositionMwms);
}

void TrafficManager::UpdateViewport(ScreenBase const & screen)
{
  m_currentModelView = {screen, true /* initialized */};

  if (!m_currentModelViewLazy.second
      || (mercator::DistanceOnEarth(m_currentModelViewLazy.first.ClipRect().Center(), screen.ClipRect().Center()) > kPositionThreshold))
  {
    m_isFeedQueueSortInvalid = true;
    m_currentModelViewLazy = m_currentModelView;
  }

  if (!IsEnabled() || IsInvalidState() || IsPausedAndNotRouting())
    return;

  if (df::GetZoomLevel(screen.GetScale()) < df::kRoadClass0ZoomLevel)
    return;

  // Request traffic.
  UpdateActiveMwms(screen.ClipRect(), m_lastDrapeMwmsByRect, m_activeDrapeMwms);
}

void TrafficManager::SubscribeOrChangeSubscription()
{
  std::set<MwmSet::MwmId> activeMwms;

  if (m_activeMwmsChanged)
  {
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_activeMwmsChanged = false;
      UniteActiveMwms(activeMwms);
    }

    {
      std::lock_guard<std::mutex> lock(m_trafficSourceMutex);
      for (auto & source : m_trafficSources)
        source->SubscribeOrChangeSubscription(activeMwms);
    }
  }
}

void TrafficManager::Unsubscribe()
{
  std::lock_guard<std::mutex> lock(m_trafficSourceMutex);
  for (auto & source : m_trafficSources)
    source->Unsubscribe();
}

bool TrafficManager::RestoreCache()
{
  ASSERT(m_storage, ("m_storage cannot be null"));
  pugi::xml_document document;
  if (!m_storage->Load(document))
  {
    LOG(LWARNING, ("Failed to reload cache from storage"));
    return false;
  }

  traffxml::TraffFeed feedIn;
  traffxml::TraffFeed feedOut;
  bool hasDecoded = false;
  bool hasUndecoded = false;
  if (traffxml::ParseTraff(document, m_dataSource, feedIn))
  {
    while (!feedIn.empty())
    {
      traffxml::TraffMessage message;
      std::swap(message, feedIn.front());
      feedIn.erase(feedIn.begin());

      if (!message.IsExpired(traffxml::IsoTime::Now()))
      {
        if (!message.m_decoded.empty())
        {
          hasDecoded = true;
          // store message in cache
          m_messageCache.insert_or_assign(message.m_id, message);
        }
        else
        {
          hasUndecoded = true;
          // message needs decoding, prepare to enqueue
          feedOut.push_back(message);
        }
      }
    }
    if (!feedOut.empty())
      m_feedQueue.insert(m_feedQueue.begin(), feedOut);
    // update notification is caller’s responsibility
    return hasDecoded && !hasUndecoded;
  }
  else
  {
    LOG(LWARNING, ("An error occurred parsing the cache file"));
  }
  return false;
}

void TrafficManager::Poll()
{
  std::lock_guard<std::mutex> lock(m_trafficSourceMutex);
  for (auto & source : m_trafficSources)
    if (source->IsPollNeeded())
      source->Poll();
}

void TrafficManager::ReceiveFeed(traffxml::TraffFeed feed)
{
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_feedQueue.push_back(feed);
    m_isFeedQueueSortInvalid = true;
  }
  m_condition.notify_one();
}

void TrafficManager::RegisterSource(std::unique_ptr<traffxml::TraffSource> source)
{
  if (IsEnabled())
  {
    std::set<MwmSet::MwmId> activeMwms;

    {
      std::lock_guard<std::mutex> lock(m_mutex);
      UniteActiveMwms(activeMwms);
    }

    if (!activeMwms.empty())
      source->SubscribeOrChangeSubscription(activeMwms);
  }

  {
    std::lock_guard<std::mutex> lock(m_trafficSourceMutex);
    m_trafficSources.push_back(std::move(source));
  }

  m_isPollNeeded = IsEnabled();
}

void TrafficManager::PurgeExpiredMessages()
{
  PurgeExpiredMessagesImpl();
  OnTrafficDataUpdate();
}

bool TrafficManager::PurgeExpiredMessagesImpl()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  bool result = false;
  LOG(LINFO, ("before:", m_messageCache.size(), "message(s)"));
  traffxml::IsoTime now = traffxml::IsoTime::Now();
  for (auto it = m_messageCache.begin(); it != m_messageCache.end(); )
  {
    if (it->second.IsExpired(now))
    {
      it = m_messageCache.erase(it);
      result = true;
    }
    else
      ++it;
  }
  LOG(LINFO, ("after:", m_messageCache.size(), "message(s)"));
  return result;
}

void TrafficManager::ConsolidateFeedQueue()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_feedQueue.empty())
    return;
  for (size_t i = m_feedQueue.size() - 1; i <= 0; i--)
    for (size_t j = m_feedQueue.size() - 1; j <= 0; j--)
    {
      if (i == j)
        continue;
      for (auto it_i = m_feedQueue[i].begin(); it_i != m_feedQueue[i].end(); )
        for (auto it_j = m_feedQueue[j].end(); it_j != m_feedQueue[j].end(); )
          if (it_i->m_id == it_j->m_id)
          {
            // dupe, remove older
            if (it_i->m_updateTime < it_j->m_updateTime)
            {
              // standard case: i has the newer one
              ++it_i;
              it_j = m_feedQueue[j].erase(it_j);
            }
            else if (it_i->m_updateTime < it_j->m_updateTime)
            {
              // j has the newer one
              it_i = m_feedQueue[i].erase(it_i);
              ++it_j;
            }
            else if (i > j)
            {
              // same time, but feed i was received after j, keep i
              ++it_i;
              it_j = m_feedQueue[j].erase(it_j);
            }
            else
            {
              // same time, but feed j was received after i, keep j
              ASSERT(i != j, ());
              it_i = m_feedQueue[i].erase(it_i);
              ++it_j;
            }
          }
    }
  // remove empty feeds from the beginning of the queue
  while (!m_feedQueue.empty() && m_feedQueue.front().empty())
    m_feedQueue.erase(m_feedQueue.begin());
  // merge everything into the first vector
  for (size_t i = 1; i < m_feedQueue.size(); i++)
  {
    if (m_feedQueue[i].empty())
      continue;
    m_feedQueue[0].insert(m_feedQueue[0].end(),
              std::make_move_iterator(m_feedQueue[i].begin()),
              std::make_move_iterator(m_feedQueue[i].end()));
    m_feedQueue[i].clear();
  }
  // remove empty feeds (any feeds after the first one are empty at this point)
  if (!m_feedQueue.empty())
  {
    if (!m_feedQueue.front().empty())
      m_feedQueue.resize(1);
    else
      m_feedQueue.clear();
  }
}

void TrafficManager::DecodeFirstMessage()
{
  traffxml::TraffMessage message;
  {
    // Lock the mutex while iterating over the feed queue
    std::lock_guard<std::mutex> lock(m_mutex);
    // remove empty feeds from the beginning of the queue
    while (!m_feedQueue.empty() && m_feedQueue.front().empty())
      m_feedQueue.erase(m_feedQueue.begin());
    // if we have no more feeds, return (nothing to do)
    if (m_feedQueue.empty())
      return;
    if (m_isFeedQueueSortInvalid
        && (m_currentPositionLazy.second || m_currentModelViewLazy.second))
    {
      std::sort(m_feedQueue.front().begin(), m_feedQueue.front().end(),
                [this](const traffxml::TraffMessage & a, const traffxml::TraffMessage & b){
        // return a < b
        // put messages first which decode quickly: cancellations, and updates with same location
        if (a.m_cancellation)
          return !b.m_cancellation;
        else if (b.m_cancellation)
          return false;
        auto aIt = m_messageCache.find(a.m_id);
        auto bIt = m_messageCache.find(b.m_id);
        if ((aIt != m_messageCache.end()) && (aIt->second.m_location == a.m_location))
          return !((bIt != m_messageCache.end()) && (bIt->second.m_location == b.m_location));
        else if ((bIt != m_messageCache.end()) && (bIt->second.m_location == b.m_location))
          return false;
        // sort by shortest distance between reference point and position/viewport
        std::vector<ms::LatLon> locations;
        if (m_currentPositionLazy.second)
          locations.push_back(mercator::ToLatLon(m_currentPositionLazy.first.m_position));
        if (m_currentModelViewLazy.second)
          locations.push_back(mercator::ToLatLon(m_currentModelViewLazy.first.ClipRect().Center()));
        double aDist = 4.5e+7;
        double bDist = 4.5e+7;
        for (auto const & [message, dist] : { std::pair{a, std::ref(aDist)}, {b, std::ref(bDist)} })
        {
          // messages without location first
          if (!message.m_location)
          {
            dist.get() = 0;
            continue;
          }
          // for simplification, we are skipping the via point
          for (auto const & point : { message.m_location.value().m_from,
              message.m_location.value().m_at,
              message.m_location.value().m_to })
          {
            if (!point)
              continue;
            for (auto const & location : locations)
            {
              auto newdist = ms::DistanceOnEarth(point.value().m_coordinates, location);
              if (newdist < dist.get())
                dist.get() = newdist;
            }
          }
        }
        return aDist < bDist;
      });
      m_isFeedQueueSortInvalid = false;
    }
    // retrieve the first message from the first feed, remove it from the feed
    std::swap(message, m_feedQueue.front().front());
    m_feedQueue.front().erase(m_feedQueue.front().begin());
    // if the feed has no more messages, erase it (eager erase, as an empty queue is used as a condition later)
    if (m_feedQueue.front().empty())
      m_feedQueue.erase(m_feedQueue.begin());
  }

  {
    std::lock_guard<std::mutex> lock(m_mutex);

    // check if message is actually newer
    auto it = m_messageCache.find(message.m_id);
    bool process = (it == m_messageCache.end());
    if (!process)
      process = (it->second.m_updateTime < message.m_updateTime);
    if (!process)
    {
      LOG(LINFO, ("message", message.m_id, "is already in cache, skipping"));
      return;
    }
  }

  LOG(LINFO, (" ", message.m_id, ":", message));
  m_traffDecoder->DecodeMessage(message);
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    // store message in cache
    m_messageCache.insert_or_assign(message.m_id, message);

    for (auto & replaced : message.m_replaces)
    {
      auto it = m_messageCache.find(replaced);
      if (it != m_messageCache.cend())
        m_messageCache.erase(it);
    }
  }
  /*
   * TODO detect if we can do a quick update:
   *  - new message which does not replace any existing message
   *  - coloring “wins” over replaced message:
   *    - contains all the segments of the previous message (always true when location is the same)
   *    - speed groups are the same or lower as in previous message (always true when all members of
   *      traffic impact are unchanged or have worsened) – for this purpose, closure is considered
   *      lower than any other speed group
   *  In this case, run:
   *      traffxml::MergeMultiMwmColoring(message.m_decoded, m_allMwmColoring);
   *  Otherwise, set a flag indicating we need to process coloring in full.
   */
}

void TrafficManager::ThreadRoutine()
{
  // initially, treat last purge and drape/observer update as having just happened
  auto lastPurged = steady_clock::now();
  m_lastDrapeUpdate = steady_clock::now();
  m_lastObserverUpdate = steady_clock::now();

  while (WaitForRequest())
  {
    if (!IsEnabled() || IsPausedAndNotRouting())
      continue;

    /*
     * Whether to call OnTrafficDataUpdate() at the end of the current round.
     * The logic may fail to catch cases in which the first message in queue replaces another
     * message without changing coloring. This would usually occur in a larger feed, where other
     * messages would likely require an announcement, making this a minor issue. A single round
     * (after a timeout) with no messages expired and an empty queue would not trigger an update.
     */
    bool hasUpdates = false;

    if (!IsTestMode())
    {
      if (steady_clock::now() - lastPurged >= kPurgeInterval)
      {
        lastPurged = steady_clock::now();
        hasUpdates |= PurgeExpiredMessagesImpl();
      }

      LOG(LINFO, ("active MWMs changed:", m_activeMwmsChanged, ", poll needed:", m_isPollNeeded));

      // this is a no-op if active MWMs have not changed
      SubscribeOrChangeSubscription();

      /*
       * Poll sources if needed.
       * m_isPollNeeded may be set by WaitForRequest() and set/unset by SubscribeOrChangeSubscription().
       */
      if (m_isPollNeeded)
      {
        m_lastResponseTime = steady_clock::now();
        m_isPollNeeded = false;
        Poll();
      }
    }
    LOG(LINFO, (m_feedQueue.size(), "feed(s) in queue"));

    // consolidate feed queue (remove older messages in favor of newer ones)
    ConsolidateFeedQueue();
    hasUpdates |= !m_feedQueue.empty();

    // decode one message and add it to the cache
    DecodeFirstMessage();

    // set new coloring for MWMs
    // `m_mutex` is obtained inside the method, no need to do it here
    if (hasUpdates)
      OnTrafficDataUpdate();
  }
  Unsubscribe();
}

bool TrafficManager::WaitForRequest()
{
  std::unique_lock<std::mutex> lock(m_mutex);

  /*
   * if we got terminated, return false immediately
   * (don’t wait until sleep, we might not get much sleep if we’re busy processing a long feed)
   */
  if (!m_isRunning)
    return false;

  if (IsEnabled() && !IsPausedAndNotRouting())
  {
    // if we have feeds in the queue, return immediately
    if (!m_feedQueue.empty())
    {
      LOG(LINFO, ("feed queue not empty, returning immediately"));
      return true;
    }

    if (!IsTestMode())
    {
      // if update interval has elapsed, return immediately
      auto const currentTime = steady_clock::now();
      auto const passedSeconds = currentTime - m_lastResponseTime;
      if (passedSeconds >= kUpdateInterval)
      {
        LOG(LINFO, ("last response was", passedSeconds, "ago, returning immediately"));
        m_isPollNeeded = true;
        return true;
      }
    }
  }

  LOG(LINFO, ("nothing to do for now, waiting for timeout or notification"));
  bool const timeout = !m_condition.wait_for(lock, kUpdateInterval, [this]
  {
    // return false to continue waiting, true for any condition we want to process immediately
    // return immediately if we got terminated
    if (!m_isRunning)
      return true;
    // otherwise continue waiting if we are paused or disabled
    if (!IsEnabled() || m_isPaused)
      return false;
    return (m_activeMwmsChanged && !IsTestMode()) || !m_feedQueue.empty();
  });

  // check again if we got terminated while waiting (or woken up because we got terminated)
  if (!m_isRunning)
    return false;

  // this works as long as wait timeout is at least equal to the poll interval
  if (IsEnabled() && !m_isPaused)
    m_isPollNeeded |= timeout;

  LOG(LINFO, ("timeout:", timeout, "active MWMs changed:", m_activeMwmsChanged, "test mode:", IsTestMode()));
  return true;
}

void TrafficManager::OnTrafficDataUpdate()
{
  bool feedQueueEmpty = false;

  {
    std::lock_guard<std::mutex> lock(m_mutex);
    feedQueueEmpty = m_feedQueue.empty();
  }
  // Whether to notify the Drape engine of the update.
  bool notifyDrape = (feedQueueEmpty);

  // Whether to notify the observer of the update.
  bool notifyObserver = (feedQueueEmpty);

  // Whether to update the cache file.
  bool updateStorage = (feedQueueEmpty);

  if (!feedQueueEmpty)
  {
    auto const currentTime = steady_clock::now();
    auto const drapeAge = currentTime - m_lastDrapeUpdate;
    auto const storageAge = currentTime - m_lastStorageUpdate;
    notifyDrape = (drapeAge >= kDrapeUpdateInterval);
    updateStorage = (storageAge >= kStorageUpdateInterval);
    if (!IsObserverInhibited())
    {
      /*
       * To avoid resetting the route over and over again while building, inhibit periodic updates
       * to the router while a route is being built. Periodic updates will resume once the route is
       * fully built. During route calculation, traffic updates are sent only if the queue is empty.
       * TODO test this with TMC, where messages arrive one by one, or in short bursts, and the
       * queue may run empty multiple times while a route is being calculated.
       */
      auto const observerAge = currentTime - m_lastObserverUpdate;
      notifyObserver = (observerAge >= kObserverUpdateInterval);
    }
  }

  if (!m_storage || IsTestMode())
    updateStorage = false;

  if (updateStorage)
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    pugi::xml_document document;

    traffxml::GenerateTraff(m_messageCache, document);
    if (!m_storage->Save(document))
      LOG(LWARNING, ("Storing message cache to file failed."));

    m_lastStorageUpdate = steady_clock::now();
  }

  if (m_trafficUpdateCallbackFn)
    m_trafficUpdateCallbackFn.value()(feedQueueEmpty);

  if (!notifyDrape && !notifyObserver)
    return;

  LOG(LINFO, ("Announcing traffic update, notifyDrape:", notifyDrape, "notifyObserver:", notifyObserver));

  /*
   * TODO introduce a flag to indicate we need to fully reprocess coloring, skip if it is false.
   * The flag would get set when messages get deleted (including any clear/purge operations),
   * or when a new message is added without indicating a simplified update in `DecodeFirstMessage()`.
   * When we reprocess coloring in full (the block below), reset this flag.
   */
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_allMwmColoring.clear();
    for (const auto & [id, message] : m_messageCache)
      traffxml::MergeMultiMwmColoring(message.m_decoded, m_allMwmColoring);
  }

  /*
   * Much of this code is copied and pasted together from old MWM code, with some minor adaptations:
   *
   * ForEachMwm (not the body) is from RequestTrafficData() (now RequestTrafficSubscription()),
   * modification: cycle over all MWMs (active or not).
   * Handling dead MWMs and traffic cache lookup is original code.
   * TrafficInfo construction is taken fron ThreadRoutine(), with modifications (different constructor).
   * The remainder of the loop is from OnTrafficDataResponse(traffic::TrafficInfo &&), with some modifications
   * (removed CacheEntry logic; deciding whether to notify a component and managing timestamps is original code).
   * Existing coloring deletion (if there is no new coloring) is original code.
   */
  ForEachMwm([this, notifyDrape, notifyObserver](std::shared_ptr<MwmInfo> info) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (info->GetCountryName().starts_with(WORLD_FILE_NAME))
      return;

    MwmSet::MwmId const mwmId(info);

    auto tcit = m_allMwmColoring.find(mwmId);
    if (!mwmId.IsAlive())
    {
      // MWM was deleted or replaced during decoding
      if (tcit != m_allMwmColoring.end())
        m_allMwmColoring.erase(tcit);
      return;
    }
    if (tcit != m_allMwmColoring.end())
    {
      traffic::TrafficInfo::Coloring coloring = tcit->second;
      LOG(LINFO, ("Setting new coloring for", mwmId, "with", coloring.size(), "entries"));
      traffic::TrafficInfo info(mwmId, std::move(coloring));

      if (notifyDrape)
      {
        /*
         * TODO calling ClearTrafficCache before UpdateTraffic is a workaround for a bug in the
         * Drape engine: some segments found in the old coloring but not in the new one may get
         * left behind. This was not a problem for MapsWithMe as the set of segments never
         * changed, but is an issue wherever the segment set is dynamic. Workaround is to clear
         * before sending an update. Ultimately, the processing logic for UpdateTraffic needs to
         * be fixed, but the code is hard to read (involves multiple messages getting thrown back
         * and forth between threads).
         */
        m_drapeEngine.SafeCall(&df::DrapeEngine::ClearTrafficCache,
                               static_cast<MwmSet::MwmId const &>(mwmId));
        m_drapeEngine.SafeCall(&df::DrapeEngine::UpdateTraffic,
                               static_cast<traffic::TrafficInfo const &>(info));
        m_lastDrapeUpdate = steady_clock::now();
      }

      if (notifyObserver)
      {
        // Update traffic colors for routing.
        m_routingSession.OnTrafficInfoAdded(std::move(info));
        m_lastObserverUpdate = steady_clock::now();
      }
    }
    else
    {
      if (notifyDrape)
      {
        m_drapeEngine.SafeCall(&df::DrapeEngine::ClearTrafficCache,
                               static_cast<MwmSet::MwmId const &>(mwmId));
        m_lastDrapeUpdate = steady_clock::now();
      }

      if (notifyObserver)
      {
        // Update traffic colors for routing.
        m_routingSession.OnTrafficInfoRemoved(mwmId);
        m_lastObserverUpdate = steady_clock::now();
      }
    }
  });
}

void TrafficManager::GetActiveMwms(std::set<MwmSet::MwmId> & activeMwms)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  UniteActiveMwms(activeMwms);
}

void TrafficManager::UniteActiveMwms(std::set<MwmSet::MwmId> & activeMwms) const
{
  activeMwms.insert(m_activeDrapeMwms.cbegin(), m_activeDrapeMwms.cend());
  activeMwms.insert(m_activePositionMwms.cbegin(), m_activePositionMwms.cend());
  activeMwms.insert(m_activeRoutingMwms.cbegin(), m_activeRoutingMwms.cend());
}

bool TrafficManager::IsEnabled() const
{
  return m_state != TrafficState::Disabled;
}

void TrafficManager::SetHttpTraffSource(bool enabled, std::string url)
{
  if (IsTestMode())
    return;

  {
    std::lock_guard<std::mutex> lock(m_trafficSourceMutex);

    for (auto it = m_trafficSources.begin(); it != m_trafficSources.end(); )
      if (traffxml::HttpTraffSource* httpSource = dynamic_cast<traffxml::HttpTraffSource*>(it->get()))
      {
        httpSource->Close();
        m_trafficSources.erase(it);
      }
      else
        ++it;
  }
  if (enabled)
    traffxml::HttpTraffSource::Create(*this, url);
}

void TrafficManager::RemoveTraffSourceIf(const std::function<bool(traffxml::TraffSource *)> &pred)
{
  std::lock_guard<std::mutex> lock(m_trafficSourceMutex);

  for (auto it = m_trafficSources.begin(); it != m_trafficSources.end(); )
    if (pred(it->get()))
      m_trafficSources.erase(it);
    else
      ++it;
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
  RecalculateSubscription(false);
}

void TrafficManager::SetSimplifiedColorScheme(bool simplified)
{
  m_hasSimplifiedColorScheme = simplified;
  m_drapeEngine.SafeCall(&df::DrapeEngine::SetSimplifiedTrafficColors, simplified);
}

void TrafficManager::SetTestMode()
{
  if (!m_canSetMode)
  {
    LOG(LWARNING, ("Mode cannot be set once the traffic manager has been enabled"));
    return;
  }
  m_mode = Mode::Test;
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
