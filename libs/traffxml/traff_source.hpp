#pragma once

#include "traffxml/traff_model.hpp"

#include "base/thread.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/http_client.hpp"

#include <chrono>
#include <set>
#include <string>

namespace traffxml
{
class TraffSource;

/**
 * @brief Abstract class which manages TraFF sources.
 *
 * `TraffSource` and its subclasses register with `TraffSourceManager` upon creation. The
 * `TraffSourceManager` calls `TraffSource` methods to manage its subscription and poll for
 *  messages, and exposes a method to deliver message feeds.
 */
class TraffSourceManager
{
public:
  virtual ~TraffSourceManager() {}

  /**
   * @brief Retrieves all currently active MWMs.
   *
   * This method retrieves all MWMs for which traffic data is needed (viewport, current position
   * and route) and stores them in `activeMwms`.
   *
   * Implementations must ensure thread safety, so that this method can be called from any thread.
   *
   * @param activeMwms Retrieves the list of active MWMs.
   */
  virtual void GetActiveMwms(std::set<MwmSet::MwmId> & activeMwms) = 0;

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
  virtual void ReceiveFeed(traffxml::TraffFeed const & feed) = 0;

  /**
   * @brief Registers a `TraffSource`.
   * @param source The source.
   */
  virtual void RegisterSource(std::unique_ptr<TraffSource> source) = 0;
};

/**
 * @brief Abstract base class for TraFF sources.
 *
 * Subclasses encapsulate various forms of TraFF sources. The base class provides methods for
 * subscription management, message retrieval and service status.
 *
 * Any `TraffSource` method may call `TrafficManager` methods exposed through the
 * `TraffSourceManager` interface. The traffic manager must therefore ensure there is no conflict
 * between thread-synchronization mechanisms held when calling a `TraffSource` method and those
 * which may get requested when that method calls a `TraffSourceManager` method.
 *
 * Each subclass should implement a non-public constructor (private if the subclass is final,
 * protected otherwise) and a public factory method. The factory method takes the same arguments
 * as the constructor, creates an instance wrapped in a `std::unique_ptr` and registers it with
 * the `TraffSourceManager`. It can be implemented as follows:
 * ```
 * void SomeTraffSource::Create(TraffSourceManager & manager, SomeOtherArg & otherArg)
 * {
 *   std::unique_ptr<SomeTraffSource> source = std::unique_ptr<SomeTraffSource>(new SomeTraffSource(manager, otherArg));
 *   manager.RegisterSource(std::move(source));
 * }
 * ```
 *
 * Each subclass must provide implementations for `Subscribe()`, `ChangeSubscription()`,
 * `Unsubscribe()`, `IsPollNeeded()` and `Poll()`.
 *
 * Most of these methods can be called from any thread, including the UI thread (see documentation
 * of individual methods for details). This has two implications:
 *
 * Subclasses must ensure thread safety for methods they implement, in particular regarding access
 * to shared members. This can be done by locking `m_mutex`.
 *
 * Also, methods should not block or perform lengthy operations. Network operations must be
 * delegated to a separate thread (attempting a network operation on the UI thread will cause the
 * application to be killed on Android).
 *
 * This class provides various protected members which subclasses can build upon. These include a
 * reference to the `TraffSourceManager`, a mutex for thread-safe access, a subscription ID,
 * timestamps for the last request and response, as well as for the next request, a retry count
 * for failed operations, and an indication of a pending request.
 */
class TraffSource
{
public:
  /**
   * @brief Whether traffic data is available.
   *
   * The default value upon creating a new instance should be `Unknown`. After that, the value
   * should be changed based on the result of the last TraFF operation, as detailed below:
   *
   * `OK` changes the status from `Unknown`, or any error which would be resolved by the last
   * operation, to `IsAvailable`.
   *
   * `INVALID` indicates a condition which should be treated as a bug, either in the source or its
   * backend. It should generate a log entry, and changes the status to `Error`.
   *
   * `SUBSCRIPTION_REJECTED` changes the status to `SubscriptionRejected`.
   *
   * `NOT_COVERED` changes the status to `NotCovered`.
   *
   * `PARTIALLY_COVERED` has the same effect as `OK`.
   *
   * `SUBSCRIPTION_UNKNOWN` should be handled by clearing the subscription ID and resubscribing,
   * then setting the status based on the result of the new subscription.
   *
   * `INTERNAL_ERROR` changes the status to `Error`.
   *
   * If the source does not seem to be connected to a valid backend (e.g. if a HTTP source responds
   * with an HTTP error), the status should be changed to `Error`.
   *
   * If a TraFF `GET_CAPABILITIES` request returns a minimum version higher than supported by this
   * application, the status should be changed to `ExpiredApp` and no further requests to the source
   * should be attempted.
   *
   * @todo Should `PARTIALLY_COVERED`, or `GET_CAPABILITIES` reporting a target version higher than
   * supported, be stored in the class instance?
   */
  enum class Availability
  {
    /**
     * The source is working normally.
     * This status is reached after the first request was made, if it is successful.
     */
    IsAvailable,
    /**
     * The source, or its backend, rejected the subscription.
     * This may happen for various reasons, possibly because the requested area was too large.
     * An existing subscription ID (if any) remains valid, but no poll operations should be
     * attempted until the subscription is changed successfully.
     */
    SubscriptionRejected,
    /**
     * The requested area is not covered by the source.
     * An existing subscription ID (if any) remains valid, but poll operations will not return any
     * messages until the subscription is changed successfully.
     */
    NotCovered,
    /**
     * The source has reported an internal error, has reported an invalid request or returned
     * invalid data.
     * The failed operation should be retried at a resonably chosen interval. After the source
     * resumes normal operation, previously issued subscription IDs may no longer be valid (in which
     * case the caller should attempt to resubscribe) and/or messages may be repeated.
     */
    Error,
    /** The app does not support the minimum TraFF version required by the source. */
    ExpiredApp,
    /** No request was made yet. */
    Unknown
  };

  virtual ~TraffSource() {}

  /**
   * @brief Ensures we have a subscription covering the MWMs indicated.
   *
   * This method subscribes to a traffic service if not already subscribed, or changes the existing
   * subscription otherwise.
   *
   * The default implementation acquires the mutex before running the following code:
   *
   * ```
   * if (!IsSubscribed())
   *   Subscribe(mwms);
   * else
   *   ChangeSubscription(mwms);
   * ```
   *
   * Therefore, `IsSubscribed()`, `Subscribe()` and `ChangeSubscription()` need not (and should not)
   * acquire the mutex on their own.
   *
   * @param mwms The new set of MWMs for which data is needed.
   */
  virtual void SubscribeOrChangeSubscription(std::set<MwmSet::MwmId> const & mwms);

  /**
   * @brief Whether this source should be polled.
   *
   * Prior to calling `Poll()` on a source, the caller should always first call `IsPollNeeded()` and
   * poll the source only if the result is true.
   *
   * It is up to the source to decide when to return true or false. Typically a source would return
   * false if another request is still pending, a predefined poll interval has not yet elapsed since
   * the previous successful response, during the retry interval following an error, or if an error
   * is not recoverable (such as `ExpiredApp`). In all other case it would return true.
   *
   * This method is only called from the `TrafficManager` worker thread.
   *
   * @return true if the source should be polled, false if not.
   */
  virtual bool IsPollNeeded() = 0;

  /**
   * @brief Polls the traffic service for updates.
   *
   * For sources which reliably push data, this implementation may do nothing.
   *
   * It is up to the caller to call `IsPollNeeded()` prior to calling this function, and use its
   * result to decide whether or not to poll, or to force a poll operation.
   *
   * Sources should handle cases in which the backend responds with `SUBSCRIPTION_UNKNOWN`, usually
   * by deleting the subscription ID and resubscribing to the set of active MWMs. The set of active
   * MWMs can be retrieved by calling `m_manager.GetActiveMwms()`.
   *
   * This method is only called from the `TrafficManager` worker thread.
   */
  virtual void Poll() = 0;

  /**
   * @brief Unsubscribes from a traffic service we are subscribed to.
   *
   * Unsubscribing without being subscribed is a no-op.
   */
  virtual void Unsubscribe() = 0;

protected:
  /**
   * @brief Constructs a new `TraffSource`.
   * @param manager The `TrafficSourceManager` instance to register the source with.
   */
  TraffSource(TraffSourceManager & manager);

  /**
   * @brief Returns a TraFF filter list for a set of MWMs.
   *
   * @param mwms The MWMs for which a filter list is to be created.
   * @return A `filter_list` in XML format.
   */
  static std::string GetMwmFilters(std::set<MwmSet::MwmId> const & mwms);

  /**
   * @brief Subscribes to a traffic service.
   *
   * If the default implementation of `SubscribeOrChangeSubscription()` is used, `m_mutex` is
   * acquired before this method is called, and implementations do not need to (and should not)
   * acquire it again. Any other calls to this method must be protected by acquiring `m_mutex`.
   *
   * @param mwms The MWMs for which data is needed.
   */
  virtual void Subscribe(std::set<MwmSet::MwmId> const & mwms) = 0;

  /**
   * @brief Changes an existing traffic subscription.
   *
   * If the default implementation of `SubscribeOrChangeSubscription()` is used, `m_mutex` is
   * acquired before this method is called, and implementations do not need to (and should not)
   * acquire it again. Any other calls to this method must be protected by acquiring `m_mutex`.
   *
   * Sources should handle cases in which the backend responds with `SUBSCRIPTION_UNKNOWN`, usually
   * by deleting the subscription ID and resubscribing to `mwms`. Asynchronous implementations, in
   * which `mwms` may no longer be available when the operation completes, can retrieve the set of
   * active MWMs can be retrieved by calling `m_manager.GetActiveMwms()`.
   *
   * @param mwms The new set of MWMs for which data is needed.
   */
  virtual void ChangeSubscription(std::set<MwmSet::MwmId> const & mwms) = 0;

  /**
   * @brief Whether we are currently subscribed to a traffic service.
   *
   * If the default implementation of `SubscribeOrChangeSubscription()` is used, `m_mutex` is
   * acquired before this method is called, and implementations do not need to (and should not)
   * acquire it again. Any other calls to this method must be protected by acquiring `m_mutex`.
   *
   * @return true if subscribed, false if not.
   */
  virtual bool IsSubscribed() { return !m_subscriptionId.empty(); }

  TraffSourceManager & m_manager;

  /**
   * @brief Mutex for access to shared members.
   *
   * Any access to members shared between threads must be protected by obtaining this mutex first.
   */
  std::mutex m_mutex;

  /**
   * @brief The subscription ID received from the backend.
   *
   * An empty subscription ID means no subscription.
   */
  std::string m_subscriptionId;

  /**
   * @brief When the last update request occurred.
   *
   * This timestamp is the basis for determining whether an update is needed.
   *
   * It is initially in the past. Subclasses that use it should update it whenever a request is made.
   */
  std::atomic<std::chrono::time_point<std::chrono::steady_clock>> m_lastRequestTime;

  /**
   * @brief When the last response was received.
   *
   * This timestamp is the basis for determining whether a network request timed out, or if data is
   * outdated.
   *
   * It is initially in the past. Subclasses that use it should update it whenever a response to a
   * request is received.
   */
  std::atomic<std::chrono::time_point<std::chrono::steady_clock>> m_lastResponseTime;

  /**
   * @brief When the next request should be made.
   *
   * This timestamp is initiated to current time and updated when a request is made, or a response
   * is received.
   *
   * It is initially in the present. Subclasses that use it should update it on every request or
   * response, setting it a defined timespan into the future.
   */
  std::atomic<std::chrono::time_point<std::chrono::steady_clock>> m_nextRequestTime = std::chrono::steady_clock::now();

  /**
   * @brief The number of failed traffic requests for this source.
   *
   * Reset when a request is successful.
   */
  std::atomic<int> m_retriesCount = 0;

  /**
   * @brief Whether a request is currently pending for this source.
   *
   * Set to `true` when a request is scheduled, reverted to `false` when a response is received or
   * the request fails.
   */
  std::atomic<bool> m_isWaitingForResponse = false;

  /**
   * @brief The last reported availability of the traffic source.
   *
   * See the documentation of `Availability` for possible values and their meanings.
   *
   * Availability is `Unknown` until a result for the first request (positive or negative) has been
   * received. Subclasses must update this value, ensuring it always correctly reflects the status
   * of the source.
   */
  std::atomic<Availability> m_lastAvailability = Availability::Unknown;

private:
  DISALLOW_COPY(TraffSource);
};

/**
 * @brief A mock TraFF source.
 *
 * This source will accept any and all subscription requests and return a static subscription ID.
 * Polling will return a static set of messages.
 */
class MockTraffSource : public TraffSource
{
public:
  /**
   * @brief Creates a new `MockTraffSource` instance and registers it with the traffic manager.
   *
   * @param manager The traffic manager to register the new instance with
   */
  static void Create(TraffSourceManager & manager);

  /**
   * @brief Subscribes to a traffic service.
   *
   * @param mwms The MWMs for which data is needed.
   */
  virtual void Subscribe(std::set<MwmSet::MwmId> const & mwms) override;

  /**
   * @brief Changes an existing traffic subscription.
   *
   * @param mwms The new set of MWMs for which data is needed.
   */
  virtual void ChangeSubscription(std::set<MwmSet::MwmId> const & mwms) override;

  /**
   * @brief Unsubscribes from a traffic service we are subscribed to.
   */
  virtual void Unsubscribe() override;

  /**
   * @brief Whether this source should be polled.
   *
   * Prior to calling `Poll()` on a source, the caller should always first call `IsPollNeeded()` and
   * poll the source only if the result is true.
   *
   * This implementation uses `m_nextRequestTime` to determine when the next poll is due. When a
   * feed is received, `m_nextRequestTime` is set to a point in time 5 minutes in the future. As
   * long as `m_nextRequestTime` is in the future, this method returns false.
   *
   * @return true if the source should be polled, false if not.
   */
  virtual bool IsPollNeeded() override;

  /**
   * @brief Polls the traffic service for updates.
   */
  virtual void Poll() override;

protected:
  /**
   * @brief Constructs a new `MockTraffSource`.
   * @param manager The `TrafficSourceManager` instance to register the source with.
   */
  MockTraffSource(TraffSourceManager & manager);

private:
  /**
   * @brief The update interval, 5 minutes.
   */
  static auto constexpr m_updateInterval = std::chrono::minutes(5);
};

/**
 * @brief A TraFF source backed by a HTTP[S] server.
 */
class HttpTraffSource : public TraffSource
{
public:
  /**
   * @brief Creates a new `HttpTraffSource` instance and registers it with the traffic manager.
   *
   * @param manager The traffic manager to register the new instance with
   * @param url The URL for the TraFF service API.
   */
  static void Create(TraffSourceManager & manager, std::string const & url);

  /**
   * @brief Prepares the HTTP traffic source for unloading.
   *
   * If there is still an active subscription, it unsubscribes, but without processing the result
   * received from the service. Otherwise, teardown is a no-op.
   */
  // TODO move this to the parent class and override it here?
  void Close();

  /**
   * @brief Subscribes to a traffic service.
   *
   * @param mwms The MWMs for which data is needed.
   */
  virtual void Subscribe(std::set<MwmSet::MwmId> const & mwms) override;

  /**
   * @brief Changes an existing traffic subscription.
   *
   * @param mwms The new set of MWMs for which data is needed.
   */
  virtual void ChangeSubscription(std::set<MwmSet::MwmId> const & mwms) override;

  /**
   * @brief Unsubscribes from a traffic service we are subscribed to.
   */
  virtual void Unsubscribe() override;

  /**
   * @brief Whether this source should be polled.
   *
   * Prior to calling `Poll()` on a source, the caller should always first call `IsPollNeeded()` and
   * poll the source only if the result is true.
   *
   * @todo Document how the result is calculated. For example:
   * This implementation uses `m_nextRequestTime` to determine when the next poll is due. When a
   * feed is received, `m_nextRequestTime` is set to a point in time 5 minutes in the future. As
   * long as `m_nextRequestTime` is in the future, this method returns false.
   *
   * @return true if the source should be polled, false if not.
   */
  virtual bool IsPollNeeded() override;

  /**
   * @brief Polls the traffic service for updates.
   */
  virtual void Poll() override;

protected:
  /**
   * @brief Constructs a new `HttpTraffSource`.
   * @param manager The `TrafficSourceManager` instance to register the source with.
   * @param url The URL for the TraFF service API.
   */
  HttpTraffSource(TraffSourceManager & manager, std::string const & url);

private:
  /**
   * @brief Processes a TraFF feed.
   * @param feed The feed.
   */
  void OnFeedReceived(TraffFeed const & feed);

  /**
   * @brief Processes the response to a subscribe request.
   * @param response The response to the subscribe operation.
   */
  void OnSubscribeResponse(TraffResponse const & response);

  /**
   * @brief Processes the response to a change subscription request.
   * @param response The response to the change subscription operation.
   */
  void OnChangeSubscriptionResponse(TraffResponse const & response);

  /**
   * @brief Processes the response to an unsubscribe request.
   * @param response The response to the unsubscribe operation.
   */
  void OnUnsubscribeResponse(TraffResponse const & response);

  /**
   * @brief Processes the response to a poll request.
   * @param response The response to the poll operation.
   */
  void OnPollResponse(TraffResponse const & response);

  /**
   * @brief The update interval, 5 minutes.
   */
  static auto constexpr m_updateInterval = std::chrono::minutes(5);

  /**
   * @brief The URL for the TraFF service.
   */
  const std::string m_url;
};
}  // namespace traffxml
