#pragma once

#include "traffic/traffic_info.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/pointers.hpp"

#include "indexer/mwm_set.hpp"

#include "routing/routing_session.hpp"

#include "storage/country_info_getter.hpp"

#include "traffxml/traff_decoder.hpp"
#include "traffxml/traff_model.hpp"
#include "traffxml/traff_source.hpp"
#include "traffxml/traff_storage.hpp"

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

class TrafficManager final : public traffxml::TraffSourceManager
{
public:
  using CountryInfoGetterFn = std::function<storage::CountryInfoGetter const &()>;
  using CountryParentNameGetterFn = std::function<std::string(std::string const &)>;
  using TrafficUpdateCallbackFn = std::function<void(bool)>;

  /**
   * @brief Global state of traffic information.
   */
  /*
   * TODO clean out obsolete states.
   * Only `Disabled` and `Enabled` are currently used, but some might be reactivated in the future
   * and platforms (android/iphone) still evaluate all states.
   * `ExpiredData` is definitely obsolete, as traffic data is no longer dependent on a particular
   * map version, but still evaluated by android/iphone code.
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

  /**
   * @brief The mode for the traffic manager.
   *
   * Future versions may introduce further test modes. Therefore, always use `TrafficManager::IsTestMode()`
   * to verify if the traffic manager is running in test mode.
   */
  enum class Mode
  {
    /**
     * Traffic manager mode for normal operation.
     *
     * This is the default mode unless something else is explicitly set.
     */
    Normal,
    /**
     * Test mode.
     *
     * This mode will prevent the traffic manager from automatically subscribing to sources and
     * polling them. It will still receive and process push feeds.
     *
     * Future versions may introduce further behavior changes, and/or introduce more test modes.
     */
    Test
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

  TrafficManager(DataSource & dataSource,
                 CountryInfoGetterFn countryInfoGetter,
                 CountryParentNameGetterFn const & countryParentNameGetter,
                 GetMwmsByRectFn const & getMwmsByRectFn, size_t maxCacheSizeBytes,
                 routing::RoutingSession & routingSession);
  ~TrafficManager();

  void Teardown();

  /**
   * @brief Returns a copy of the cache of all currently active TraFF messages.
   *
   * For testing purposes.
   *
   * Keys are message IDs, values are messages.
   *
   * This method is safe to call from any thread.
   */
  std::map<std::string, traffxml::TraffMessage> GetMessageCache();

  TrafficState GetState() const;
  void SetStateListener(TrafficStateChangedFn const & onStateChangedFn);

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  /**
   * @brief Sets the version of the MWM used locally.
   */
  void SetCurrentDataVersion(int64_t dataVersion);

  /**
   * @brief Enables or disables the traffic manager.
   *
   * This sets the internal state and notifies the drape engine.
   *
   * Upon creation, the traffic manager is disabled. MWMs must be loaded before first enabling the
   * traffic manager.
   *
   * While disabled, the traffic manager will not update its subscription area (upon being enabled
   * again, it will do so if necessary). It will not poll any sources or process any messages. Feeds
   * added via `ReceiveFeed()` will be added to the queue but will not be processed until the
   * traffic manager is re-enabled.
   *
   * Calling this function with `enabled` identical to the current state is a no-op.
   *
   * @todo Currently, all MWMs must be loaded before calling `SetEnabled()`, as MWMs loaded after
   * that will not get picked up. We need to extend `TrafficManager` to react to MWMs being added
   * (and removed) – note that this affects the `DataSource`, not the set of active MWMs.
   * See `Framework::OnMapDeregistered()` implementation for the opposite case (MWM deregistered).
   *
   * @param enabled True to enable, false to disable
   */
  void SetEnabled(bool enabled);

  /**
   * @brief Whether the traffic manager is enabled.
   *
   * @return True if enabled, false if not
   */
  bool IsEnabled() const;

  /**
   * @brief Sets the enabled state and URL for the `HttpTraffSource`.
   *
   * If the traffic manager is in test mode, this function is a no-op.
   *
   * Otherwise this function is expected to be called only if the enabled state and/or URL have
   * actually changed. Setting both to the current state will remove the current source and create
   * a new one with identical settings.
   *
   * This function currently assumes that there is never more than one `HttpTraffSource` configured
   * at the same time.
   *
   * @param enabled Whether the HTTP TraFF source is enabled.
   * @param url The URL for the TraFF API.
   */
  void SetHttpTraffSource(bool enabled, std::string url);

  /**
   * @brief Removes all `TraffSource` instances which satisfy a predicate.
   *
   * This method iterates over all currently configured `TraffSource` instances and calls the
   * caller-suppplied predicate function `pred` on each of them. If `pred` returns true, the source
   * is removed, else it is kept.
   *
   * @todo For now, `pred` deliberately takes a non-const argument so we can do cleanup inside
   * `pred`. If we manage to move any such cleanup into the destructor of the `TraffSource` subclass
   * and get rid of any `Close()` methods in subclasses (which is preferable for other reasons as
   * well), the argument can be made const.
   *
   * @param pred The predicate function, see description.
   */
  void RemoveTraffSourceIf(const std::function<bool(traffxml::TraffSource*)>& pred);

  /**
   * @brief Starts the traffic manager.
   *
   */
  void Start();

  void UpdateViewport(ScreenBase const & screen);
  void UpdateMyPosition(MyPosition const & myPosition);

  /**
   * @brief Invalidates traffic information for the specified MWM.
   *
   * Invalidation of traffic data is always per MWM and affects locations which refer to any version
   * of this MWM, or whose enclosing rectangle overlaps with that of the MWM. The decoded segments
   * for these locations are discarded and decoded again, ensuring they are based on the new MWM.
   * The TraFF messages themselves remain unchanged.
   *
   * This method must either be called from a lambda function passed to `RunSynchronized()`,
   * or the caller must explicitly lock the private `m_mutex` prior to calling this method.
   *
   * @param mwmId The newly addded MWM.
   */
  void Invalidate(MwmSet::MwmId const & mwmId);

  void OnDestroySurface();
  void OnRecoverSurface();
  void OnMwmDeregistered(platform::LocalCountryFile const & countryFile);

  void OnEnterForeground();
  void OnEnterBackground();

  void SetSimplifiedColorScheme(bool simplified);
  bool HasSimplifiedColorScheme() const { return m_hasSimplifiedColorScheme; }

  /**
   * @brief Whether the traffic manager is operating in test mode.
   */
  bool IsTestMode() { return m_mode != Mode::Normal; }

  /**
   * @brief Switches the traffic manager into test mode.
   *
   * The mode can only be set before the traffic manager is first enabled. After that, this method
   * will log a warning but otherwise do nothing.
   *
   * In test mode, the traffic manager will not subscribe to sources or poll them automatically.
   * Expired messages will not get purged automatically, but `PurgeExpiredMessages()` can be called
   * to purge expired messages once. The traffic manager will still receive and process push feeds.
   *
   * Future versions may introduce further behavior changes.
   */
  void SetTestMode();

  /**
   * @brief Processes a traffic feed.
   *
   * The feed may be a result of a pull operation, or received through a push operation.
   * (Push operations are not supported by all sources.)
   *
   * This method is safe to call from any thread.
   *
   * @param feed The traffic feed.
   */
  virtual void ReceiveFeed(traffxml::TraffFeed feed) override;

  /**
   * @brief Registers a `TraffSource`.
   * @param source The source.
   */
  virtual void RegisterSource(std::unique_ptr<traffxml::TraffSource> source) override;

  /**
   * @brief Retrieves all currently active MWMs.
   *
   * This method retrieves all MWMs in the viewport, within a certain distance of the current
   * position (if there is a valid position) or part of the route (if any), and stores them in
   * `activeMwms`.
   *
   * This method locks `m_mutex` and is therefore safe to call from any thread. Callers which
   * already hold `m_mutex` can use the private `UniteActiveMwms()` method instead.
   *
   * @param activeMwms Retrieves the list of active MWMs.
   */
  virtual void GetActiveMwms(std::set<MwmSet::MwmId> & activeMwms) override;

  /**
   * @brief Purges expired messages from the cache.
   *
   * This method is safe to call from any thread, except for the traffic worker thread.
   */
  void PurgeExpiredMessages();

  /**
   * @brief Clears the traffic message cache and feed queue.
   *
   * This is intended for testing purposes and clears the message cache, as well as the feed queue.
   * Subscriptions are not changed.
   */
  void Clear();

  /**
   * @brief Registers a callback function which gets called on traffic updates.
   *
   * Intended for testing.
   *
   * @param fn The callback function.
   */
  void SetTrafficUpdateCallbackFn(TrafficUpdateCallbackFn && fn);

  /**
   * @brief Runs a function guarded by the traffic manager mutex.
   *
   * This locks `m_mutex`, then runs `f` and releases the mutex.
   *
   * @param f
   */
  void RunSynchronized(std::function<void()> f)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    f();
  }

private:

  /**
   * @brief Recalculates the TraFF subscription area.
   *
   * The subscription area needs to be recalculated when the traffic manager goes from disabled to
   * enabled, or when it is resumed after being paused, as the subscription area is not updated
   * while the traffic manager is disabled or paused.
   *
   * If the subscription area has changed, or if `forceRenewal` is true, TraFF subscriptions are
   * renewed by calling `SubscribeOrChangeSubscription()`.
   *
   * No traffic data is discarded, but sources will be polled for an update, which may turn out
   * larger than usual if the traffic manager was in disabled/paused state for an extended period of
   * time or the subscription area has changed.
   *
   * @param forceRenewal If true, renew subscriptions even if the subscription area has not changed.
   */
  void RecalculateSubscription(bool forceRenewal);

  /**
   * @brief Ensures every TraFF source has a subscription covering all currently active MWMs.
   *
   * This method cycles through all TraFF sources in `m_trafficSources` and calls
   * `SubscribeOrChangeSubscription()` on each of them.
   */
  void SubscribeOrChangeSubscription();

  /**
   * @brief Unsubscribes from all traffic services we are subscribed to.
   *
   * This method cycles through all TraFF sources in `m_trafficSources` and calls `Unsubscribe()`
   * on each of them.
   */
  void Unsubscribe();

  /**
   * @brief Restores the message cache from file storage.
   *
   * @note The caller must lock `m_mutex` prior to calling this function, as it makes unprotected
   * changes to shared data structures.
   *
   * @note The return value indicates whether actions related to a traffic update should be taken,
   * such as notifying the routing and drape engine. It is true if at least one message with a
   * decoded location was read, and no messages without decoded locations. If messages without a
   * decoded location were read, the return value is false, as the location decoding will trigger
   * updates by itself. If errors occurred and no messages are read, the return value is also false.
   *
   * @return True if a traffic update needs to be sent, false if not
   */
  bool RestoreCache();

  /**
   * @brief Polls all traffic services for updates.
   *
   * This method cycles through all TraFF sources in `m_trafficSources` and calls `IsPollNeeded()`
   * on each of them. If this method returns true, it then calls `Poll()` on the source.
   */
  void Poll();

  /**
   * @brief Purges expired messages from the cache.
   *
   * This is the internal conterpart of `PurgeExpiredMessages()`. It is safe to call from any
   * thread. Unlike `PurgeExpiredMessages()`, it does not wake the worker thread, making it suitable
   * for use on the worker thread.
   *
   * @return true if messages were purged, false if not
   */
  bool PurgeExpiredMessagesImpl();

  /**
   * @brief Consolidates the feed queue.
   *
   * If multiple feeds in the queue have the same message ID, only the message with the newest
   * update time is kept (if two messages have the same ID and update time, the one in the feed
   * with the higher index is kept); other messages with the same ID are discarded. Empty feeds
   * are discarded.
   */
  void ConsolidateFeedQueue();

  /**
   * @brief Removes the first message from the first feed and decodes it.
   */
  void DecodeFirstMessage();

  /**
   * @brief Event loop for the traffic worker thread.
   *
   * This method runs an event loop, which blocks until woken up or a timeout equivalent to the
   * update interval elapses. It cycles through the list of MWMs for which updates have been
   * scheduled, triggering a network request for each and processing the result.
   */
  void ThreadRoutine();

  /**
   * @brief Blocks until a request for traffic data is received or a timeout expires.
   *
   * This method acts as the loop condition for `ThreadRoutine()`. It blocks until woken up or the
   * update interval expires. In the latter case, it calls `RequestTrafficData()` to insert all
   * currently active MWMs into the list of MWMs to update; otherwise, it leaves the list as it is.
   * In either case, it populates `mwms` with the list and returns.
   *
   * @return `true` during normal operation, `false` during teardown (signaling the event loop to exit).
   */
  bool WaitForRequest();

  /**
   * @brief Processes new traffic data.
   *
   * The new per-MWM colorings (preprocessed traffic information) are taken from `m_allMmColoring`.
   * `m_allMwmColoring` is rebuilt from per-message colorings in `m_messageCache` as needed.
   *
   * This method is normally called from the traffic worker thread. Test tools may also call it from
   * other threads.
   */
  void OnTrafficDataUpdate();

  /**
   * @brief Updates `activeMwms` and requests traffic data.
   *
   * The old and new list of active MWMs may refer either to those used by the rendering engine
   * (`m_lastDrapeMwmsByRect`/`m_activeDrapeMwms`) or to those around the current position.
   * (`m_lastPositionMwmsByRect`/`m_activePositionMwms`).
   *
   * The method first determines the list of MWMs overlapping with `rect`. If it is identical to
   * `lastMwmsByRect`, the method returns immediately. Otherwise, it stores the new set in
   * `lastMwmsByRect` and populates `activeMwms` with the elements.
   *
   * This method locks `m_mutex` while populating `activeMwms`. There is no need for the caller to
   * do that.
   *
   * @param rect Rectangle covering the new active MWM set.
   * @param lastMwmsByRect Set of active MWMs, see description.
   * @param activeMwms Vector of active MWMs, see description.
   */
  void UpdateActiveMwms(m2::RectD const & rect, std::vector<MwmSet::MwmId> & lastMwmsByRect,
                        std::set<MwmSet::MwmId> & activeMwms);

  // This is a group of methods that haven't their own synchronization inside.

  void ChangeState(TrafficState newState);

  bool IsInvalidState() const;

  void OnChangeRoutingSessionState(routing::SessionState previous, routing::SessionState current);

  /**
   * @brief Retrieves all currently active MWMs.
   *
   * This method retrieves all MWMs in the viewport, within a certain distance of the current
   * position (if there is a valid position) or part of the route (if any), and stores them in
   * `activeMwms`.
   *
   * The caller must hold `m_mutex` prior to calling this method. `GetActiveMwms()` is available
   * as a convenience wrapper which locks `m_mutex`, calls this method and releases it.
   *
   * @param activeMwms Retrieves the list of active MWMs.
   */
  void UniteActiveMwms(std::set<MwmSet::MwmId> & activeMwms) const;

  /**
   * @brief Pauses the traffic manager.
   *
   * Upon creation, the traffic manager is not paused.
   *
   * While the traffic manager is paused and no route is active, the traffic manager will not update
   * its subscription area (upon resuming, it will do so if necessary). It will not poll any sources
   * or process any messages. Feeds added via `ReceiveFeed()` will be added to the queue but will
   * not be processed until the traffic manager is resumed.
   *
   * Pausing and resuming is similar in effect to disabling and enabling the traffic manager, except
   * it does not change the external state, and an active route effectively overrides the paused
   * state. It is intended for internal use by the framework.
   */
  void Pause();

  /**
   * @brief Resumes the traffic manager.
   *
   * Upon creation, the traffic manager is not paused. Resuming a traffic manager that is not paused
   * is a no-op.
   *
   * Upon resume, the traffic manager will recalculate its subscription area and change its
   * subscription if necessary. It will continue processing feeds in the queue, including those
   * received before or while the traffic manager was paused.
   *
   * Pausing and resuming is similar in effect to disabling and enabling the traffic manager, except
   * it does not change the external state, and an active route effectively overrides the paused
   * state. It is intended for internal use by the framework.
   */
  void Resume();

  template <class F>
  void ForEachMwm(F && f) const
  {
    std::vector<std::shared_ptr<MwmInfo>> allMwmInfo;
    m_dataSource.GetMwmsInfo(allMwmInfo);
    std::for_each(allMwmInfo.begin(), allMwmInfo.end(), std::forward<F>(f));
  }

  template <class F>
  void ForEachActiveMwm(F && f) const
  {
    std::set<MwmSet::MwmId> activeMwms;
    UniteActiveMwms(activeMwms);
    std::for_each(activeMwms.begin(), activeMwms.end(), std::forward<F>(f));
  }

  /**
   * @brief Whether updates to the observer are currently inhibited.
   *
   * Updates are inhibited while a route calculation is in progress. In this state, the observer
   * receives traffic updates only if the queue has run empty, not if nore locations are waiting
   * to be decoded.
   *
   * Inhibtiting the observer is necessary as traffic updates during route calculation will cause
   * it to restart from scratch. Once the route has been calculated, updates will trigger a
   * recalculation, which is much faster (seconds or less).
   */
  bool IsObserverInhibited() const { return (m_routingSessionState == routing::SessionState::RouteBuilding)
      || (m_routingSessionState == routing::SessionState::RouteRebuilding); }

  /**
   * @brief Whether we are currently routing.
   */
  bool IsRouting() const { return m_routingSessionState != routing::SessionState::NoValidRoute; }

  /**
   * @brief Whether the traffic manager is paused and not routing.
   *
   * This is used to inhibit polling and message decoding.
   */
  bool IsPausedAndNotRouting() const { return m_isPaused && !IsRouting(); }

  DataSource & m_dataSource;
  CountryInfoGetterFn m_countryInfoGetterFn;
  CountryParentNameGetterFn m_countryParentNameGetterFn;
  GetMwmsByRectFn m_getMwmsByRectFn;

  /*
   * Originally this was m_observer, of type traffic::TrafficObserver. Since routing::RoutingSession
   * inherits from that class, and an interface to the routing session is needed in order to
   * determine the MWMs for which we need traffic information, the type was changed and the member
   * renamed to reflect that.
   */
  routing::RoutingSession & m_routingSession;

  /**
   * @brief Cached state of the routing session.
   *
   * `m_routingSession` methods which query the state may only be called from the GUI thread,
   * therefore we are caching this value when we get notified of a change.
   */
  routing::SessionState m_routingSessionState = routing::SessionState::NoValidRoute;

  df::DrapeEngineSafePtr m_drapeEngine;
  std::atomic<int64_t> m_currentDataVersion;

  // These fields have a flag of their initialization.
  /*
   * The lazy ones get updated only if they are not initialized, or if their new position is more
   * than a certain distance from the previously stored one.
   */
  std::pair<MyPosition, bool> m_currentPosition = {MyPosition(), false};
  std::pair<MyPosition, bool> m_currentPositionLazy = m_currentPosition;
  std::pair<ScreenBase, bool> m_currentModelView = {ScreenBase(), false};
  std::pair<ScreenBase, bool> m_currentModelViewLazy = m_currentModelView;

  /**
   * The mode in which the traffic manager is running.
   */
  Mode m_mode = Mode::Normal;

  /**
   * Whether the traffic manager accepts mode changes.
   *
   * Mode cannot be set after the traffic manager has been enabled for the first time.
   */
  bool m_canSetMode = true;

  std::atomic<TrafficState> m_state;
  TrafficStateChangedFn m_onStateChangedFn;

  bool m_hasSimplifiedColorScheme = true;

  /**
   * @brief The TraFF sources from which we get traffic information.
   *
   * Threads must lock `m_trafficSourceMutex` prior to accessing this member.
   */
  std::vector<std::unique_ptr<traffxml::TraffSource>> m_trafficSources;

  bool m_isRunning;
  std::condition_variable m_condition;

  /*
   * To determine for which MWMs we need traffic data, we need to keep track of 3 groups of MWMs:
   * those used by the renderer (i.e. in or just around the viewport), those within a certain area
   * around the current position, and those used by the routing engine (only if currently routing).
   *
   * Routing MWMs are stored as a set.
   *
   * The other groups are stored twice: as a set and as a vector. The set always holds the MWMs which
   * were last seen in use. Both get updated together when active MWMs are added or removed.
   * However, the vector is used as a reference to detect changes. Clear() clears the vector but not
   * the set, invalidating the set without destroying its contents.
   *
   * Methods which use only the set:
   *
   * * RequestTrafficSubscription(), exits if empty, otherwise cycles through the set.
   * * UniteActiveMwms(), build the list of active MWMs (used by RequestTrafficSubscription()).
   *
   * Methods which use both, but in a different way:
   *
   * * UpdateActiveMwms(), uses the vector to detect changes (not for routing MWMs). If so, it
   *   updates both vector and set, but adds MWMs to the set only if they are alive.
   */
  std::vector<MwmSet::MwmId> m_lastDrapeMwmsByRect;
  std::set<MwmSet::MwmId> m_activeDrapeMwms;
  std::vector<MwmSet::MwmId> m_lastPositionMwmsByRect;
  std::set<MwmSet::MwmId> m_activePositionMwms;
  std::set<MwmSet::MwmId> m_activeRoutingMwms;

  /**
   * @brief Whether active MWMs have changed since the last request.
   */
  bool m_activeMwmsChanged = false;

  std::atomic<bool> m_isPaused;

  /**
   * @brief Mutex for access to shared members.
   *
   * Threads which access shared members (see documentation) must lock this mutex while doing so.
   *
   * @note To access `m_trafficSource`, lock `m_trafficSourceMutex`, not this mutex.
   */
  std::mutex m_mutex;

  /**
   * @brief Mutex for access to `m_trafficSources`.
   *
   * Threads which access `m_trafficSources` must lock this mutex while doing so.
   */
  std::mutex m_trafficSourceMutex;

  /**
   * @brief Worker thread which fetches traffic updates.
   */
  threads::SimpleThread m_thread;

  /**
   * @brief When the last response was received.
   */
  std::chrono::time_point<std::chrono::steady_clock> m_lastResponseTime;

  /**
   * @brief When the last update notification to the Drape engine was posted.
   */
  std::chrono::time_point<std::chrono::steady_clock> m_lastDrapeUpdate;

  /**
   * @brief When the last update notification to the traffic observer was posted.
   */
  std::chrono::time_point<std::chrono::steady_clock> m_lastObserverUpdate;

  /**
   * @brief When the cache file was last updated.
   */
  std::chrono::time_point<std::chrono::steady_clock> m_lastStorageUpdate;

  /**
   * @brief Whether a poll operation is needed.
   *
   * Used in the worker thread to indicate we need to poll all sources. The poll operation may still
   * be inhibited for individual sources.
   */
  bool m_isPollNeeded;

  /**
   * @brief Queue of feeds waiting to be processed.
   *
   * Threads must lock `m_mutex` before accessing `m_feedQueue`, as some platforms may receive feeds
   * on multiple threads.
   */
  std::vector<traffxml::TraffFeed> m_feedQueue;

  /**
   * @brief Whether the feed queue needs to be resorted.
   *
   * Resorting is needed when a new feed is added, or the current position or the viewport center
   * has changed by more than a certain threshold.
   */
  std::atomic<bool> m_isFeedQueueSortInvalid = false;

  /**
   * @brief Cache of all currently active TraFF messages.
   *
   * Keys are message IDs, values are messages.
   *
   * Threads must lock `m_mutex` before accessing `m_messageCache`, as access can happen from
   * multiple threads (messages are added by the worker thread, `Clear()` can be called from the UI
   * thread).
   */
  std::map<std::string, traffxml::TraffMessage> m_messageCache;

  /**
   * @brief The storage instance.
   *
   * Used to persist the TraFF message cache between sessions.
   */
  std::unique_ptr<traffxml::LocalStorage> m_storage;

  /**
   * @brief The TraFF decoder instance.
   *
   * Used to decode TraFF locations into road segments on the map.
   */
  std::unique_ptr<traffxml::DefaultTraffDecoder> m_traffDecoder;

  /**
   * @brief Map between MWM IDs and their colorings.
   *
   * Threads must lock `m_mutex` before accessing `m_allMwmColoring`, as access can happen from
   * multiple threads (messages are added by the worker thread, `Clear()` can be called from the UI
   * thread).
   */
  std::map<MwmSet::MwmId, traffic::TrafficInfo::Coloring> m_allMwmColoring;

  /**
   * @brief Callback function which gets called on traffic updates.
   *
   * Intended for testing.
   */
  std::optional<TrafficUpdateCallbackFn> m_trafficUpdateCallbackFn;
};

extern std::string DebugPrint(TrafficManager::TrafficState state);
