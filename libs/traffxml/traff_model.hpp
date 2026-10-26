#pragma once

//#include "traffxml/traff_foo.hpp"

#include "geometry/latlon.hpp"

#include "indexer/mwm_set.hpp"

#include "traffic/iso_time.hpp"
#include "traffic/speed_groups.hpp"
#include "traffic/traffic_info.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace traffxml
{
constexpr uint8_t kMaxspeedNone = 255;

// TODO enum urgency

enum class Directionality
{
  OneDirection,
  BothDirections
};

enum class Fuzziness
{
  LowRes,
  MediumRes,
  EndUnknown,
  StartUnknown,
  ExtentUnknown
};

enum class Ramps
{
  None,
  All,
  Entry,
  Exit
};

enum class RoadClass
{
  Motorway,
  Trunk,
  Primary,
  Secondary,
  Tertiary,
  Other
};

enum class QuantifierType
{
  Dimension,
  Duration,
  Int,
  Ints,
  Speed,
  Temperature,
  Time,
  Weight,
  Invalid
};

/*
 * When adding a new event class to this enum, be sure to do the following:
 *
 * * in `traff_model_xml.cpp`, add the corresponding mapping to `kEventClassMap`
 * * in `traff_model.cpp`, extend `DebugPrint(EventClass)` to correctly process the new event classes
 * * in this file, add event types for this class to `EventType`
 */
enum class EventClass
{
  Invalid,
  Activity,
  Authority,
  Carpool,
  Congestion,
  Construction,
  Delay,
  Environment,
  EquipmentStatus,
  Hazard,
  Incident,
  Restriction,
  Security,
  Transport,
  Weather
};

/*
 * When adding a new event type to this enum, be sure to do the following:
 *
 * * in `traff_model_xml.cpp`, add the corresponding mapping to `kEventTypeMap`
 * * in `traff_model.cpp`:
 *   * add speed group mappings in `kEventSpeedGroupMap`, if any
 *   * add maxspeed mappings in `kEventMaxspeedMap`, if any (uncomment if needed)
 *   * add delay mappings in `kEventDelayMap`, if any
 *   * extend `DebugPrint(TraffEvent)` to correctly process the new events
 */
enum class EventType
{
  Invalid,
  // TODO Activity*, Authority*, Carpool*
  CongestionCleared,
  CongestionForecastWithdrawn,
  CongestionHeavyTraffic,
  CongestionLongQueue,
  CongestionNone,
  CongestionNormalTraffic,
  CongestionQueue,
  CongestionQueueLikely,
  CongestionSlowTraffic,
  CongestionStationaryTraffic,
  CongestionStationaryTrafficLikely,
  CongestionTrafficBuildingUp,
  CongestionTrafficCongestion,
  CongestionTrafficEasing,
  CongestionTrafficFlowingFreely,
  CongestionTrafficHeavierThanNormal,
  CongestionTrafficLighterThanNormal,
  CongestionTrafficMuchHeavierThanNormal,
  CongestionTrafficProblem,
  // TODO Construction*
  DelayClearance,
  DelayDelay,
  DelayDelayPossible,
  DelayForecastWithdrawn,
  DelayLongDelay,
  DelaySeveralHours,
  DelayUncertainDuration,
  DelayVeryLongDelay,
  // TODO Environment*, EquipmentStatus*, Hazard*, Incident*
  // TODO complete Restriction*
  RestrictionBlocked,
  RestrictionBlockedAhead,
  RestrictionCarriagewayBlocked,
  RestrictionCarriagewayClosed,
  RestrictionClosed,
  RestrictionClosedAhead,
  RestrictionEntryBlocked,
  RestrictionEntryReopened,
  RestrictionExitBlocked,
  RestrictionExitReopened,
  RestrictionOpen,
  RestrictionRampBlocked,
  RestrictionRampClosed,
  RestrictionRampReopened,
  RestrictionReopened,
  RestrictionSpeedLimit,
  RestrictionSpeedLimitLifted,
  // TODO Security*, Transport*, Weather*
};

enum class ResponseStatus
{
  /**
   * The operation was successful.
   */
  Ok,

  /**
   * The source rejected the operation as invalid
   *
   * This may happen when a nonexistent operation is attempted, or an operation is attempted with
   * incomplete or otherwise invalid data.
   *
   * @note This corresponds to TraFF status `INVALID` but was renamed here.
   * `ResponseStatus::Invalid` refers to a different kind of error.
   */
  InvalidOperation,

  /**
   * The source rejected the subscription, e.g. because the filtered region is too large.
   */
  SubscriptionRejected,

  /**
   * The source does not supply data for the requested area; the request has failed.
   */
  NotCovered,

  /**
   * The source supplies data only for a subset of the requested area; the request was successful
   * (i.e. the subscription was created or changed as requested) but the consumer should be prepared
   * to receive incomplete data.
   */
  PartiallyCovered,

  /**
   * An operation (change, push, pull) was attempted on a subscription which the recipient did not
   * recognize. On transport channels which support stable identifiers for both communication
   * parties, this is also used if a consumer attempts an operation on a subscription created by
   * another consumer.
   */
  SubscriptionUnknown,

  /**
   * The aggregator does not accept unsolicited push requests from the sensor. Reserved for future
   * versions and not used as of TraFF 0.8.
   */
  PushRejected,

  /**
   * An internal error prevented the recipient of the request from fulfilling it.
   *
   * This is either translated directly from `INTERNAL_ERROR` returned from the source, or may be
   * inferred from errors on the transport channel (e.g. HTTP errors).
   */
  InternalError,

  /**
   * An unrecognized status code.
   *
   * This is used for all situations where we got a response from the source, with no indication of
   * an error, but could not obtain a known status code from it (e.g. XML failed to parse, did not
   * contain a status code, or contained an unknown status code).
   *
   * @note Not to be confused with TraFF status `INVALID`, which maps to
   * `ResponseStatus::InvalidOperation`.
   */
  Invalid
};

/**
 * @brief Represents the impact of one or more traffic events.
 *
 * Impact can be expressed in three ways:
 *
 * Traffic may flow at a certain percentage of the posted limit, often divided in bins. This is
 * used by some traffic services which report e.g. “slow traffic”, “stationary traffic” or
 * “queues”, and maps to speed groups in a straightforward way.
 *
 * Traffic may flow at, or be restricted to, a given speed. This is common with traffic flow
 * measurement data, or with temporary speed limits. Converting this to a speed group requires
 * knowledge of the regular speed limit.
 *
 * There may be a fixed delay, expressed as a duration in time. This may happen at checkpoints,
 * at sections where traffic flow is limited or where there is single alternate-lane traffic.
 * As the routing data model does not provide for explicit delays, they have to be converted into
 * speed groups. Again, this requires knowledge of the regular travel time along the route, as well
 * as its length.
 *
 * Closures can be expressed by setting `m_speedGroup` to `traffic::SpeedGroup::TempBlock`. If that
 * is the case, the other struct members are to be ignored.
 */
struct TrafficImpact
{
  /**
   * @brief Whether two `TrafficImpact` instances are equal.
   *
   * Instances are considered equal if both have a speed group of `TempBlock`, in which case other
   * members are not compared. Otherwise, they are equal if, and only if, all three members hold
   * identical values between both instances.
   */
  // Non-member friend as member operators do not work with std::optional
  friend bool operator==(TrafficImpact const & lhs, TrafficImpact const & rhs);
  friend bool operator!=(TrafficImpact const & lhs, TrafficImpact const & rhs) { return !(lhs == rhs); }

  /**
   * @brief The speed group for the affected segments, or `traffic::SpeedGroup::Unknown` if unknown.
   */
  traffic::SpeedGroup m_speedGroup = traffic::SpeedGroup::Unknown;

  /**
   * @brief The speed limit, or speed of flowing traffic; `kMaxspeedNone` if none or unknown.
   */
  uint8_t m_maxspeed = kMaxspeedNone;

  /**
   * @brief The delay in minutes; 0 if none or unknown.
   */
  uint16_t m_delayMins = 0;
};

struct Point
{
  /**
   * @brief Whether two points are equal.
   *
   * Two points are equal if, and only if, their coordinates are. Other attributes are not compared.
   */
  // Non-member friend as member operators do not work with std::optional
  friend bool operator==(Point const & lhs, Point const & rhs);
  friend bool operator!=(Point const & lhs, Point const & rhs) { return !(lhs == rhs); }

  // TODO role?
  ms::LatLon m_coordinates = ms::LatLon::Zero();
  std::optional<float> m_distance;
  std::optional<std::string> m_junctionName;
  std::optional<std::string> m_junctionRef;
};

struct TraffLocation
{
  /**
   * @brief Whether two locations are equal.
   *
   * Two locations are equal if, and only if, they contain the same points in the same roles.
   *
   * @todo Road class and ramps are not compared, though these values are used by the decoder. Not
   * comparing these values could lead to two seemingly equal locations resolving to a different
   * path. However, given that comparison only takes place between messages with identical IDs
   * (indicating both refer to the same event at the same location), such a situation is highly
   * unlikely to occur in practice.
   */
  // Non-member friend as member operators do not work with std::optional
  friend bool operator==(TraffLocation const & lhs, TraffLocation const & rhs);
  friend bool operator!=(TraffLocation const & lhs, TraffLocation const & rhs) { return !(lhs == rhs); }

  std::optional<std::string> m_country;
  std::optional<std::string> m_destination;
  std::optional<std::string> m_direction;
  Directionality m_directionality = Directionality::BothDirections;
  std::optional<Fuzziness> m_fuzziness;
  std::optional<std::string> m_origin;
  Ramps m_ramps = Ramps::None;
  std::optional<RoadClass> m_roadClass;
  // disabled for now, optional<bool> behaves weird and we don't really need it
  //std::optional<bool> m_roadIsUrban;
  std::optional<std::string> m_roadRef;
  std::optional<std::string> m_roadName;
  std::optional<std::string> m_territory;
  std::optional<std::string> m_town;
  std::optional<Point> m_from;
  std::optional<Point> m_to;
  std::optional<Point> m_at;
  std::optional<Point> m_via;
  std::optional<Point> m_notVia;
};

struct TraffEvent
{
  EventClass m_class = EventClass::Invalid;
  EventType m_type = EventType::Invalid;
  std::optional<uint16_t> m_length;
  std::optional<uint8_t> m_probability;
  std::optional<uint16_t> m_qDurationMins;
  /*
   * TODO remaining quantifiers
   * q_dimension
   * q_int
   * q_ints
   * q_speed
   * q_temperature
   * q_time
   * q_weight
   */
  std::optional<uint8_t> m_speed;
  // TODO supplementary information
};

/**
 * @brief Global mapping from feature segments to speed groups, across all MWMs.
 *
 * `GlobalSimpleColoring` is condition-agnostic. Conditionality information must be provided outside
 * this type. Use `traffic::TrafficInfo::GlobalColoring` where full condition support is needed.
 */
using GlobalSimpleColoring = std::map<MwmSet::MwmId, traffic::TrafficInfo::SimpleColoring>;

struct TraffMessage
{
  /**
   * @brief Gets the time after which this message effectively expires.
   *
   * The effective expiration time is the latest of `m_expirationTime`, `m_startTime` and
   * `m_endTime`. `nullopt` values are ignored.
   *
   * @return The effective expiration time for the message.
   */
  traffic::IsoTime GetEffectiveExpirationTime() const;

  /**
   * @brief Returns the condition for the message.
   *
   * For unconditional messages, the result is `std::nullopt`.
   *
   * Conditions currently include the start and end time, if set. Future versions may add further
   * conditions, such as recurring schedules.
   */
  std::optional<traffic::Condition> GetCondition() const;

  /**
   * @brief Whether the message has expired.
   *
   * A message is considered to have expired if its effective expiration time (as returned by
   * `GetEffectiveExpirationTime()` refers to a point in time before `now`.
   *
   * @param now The reference time to compare to (usually current time)
   * @return True if the message has expired, false if not.
   */
  bool IsExpired(traffic::IsoTime const & now) const;

  /**
   * @brief Retrieves the traffic impact of all events.
   *
   * If the message has multiple events, the traffic impact is determined separately for each
   * event and then aggregated. Aggregation takes the most restrictive value in each category
   * (speed group, maxspeed, delay).
   *
   * If the aggregated traffic impact includes `SpeedGroup::TempBlock`, its other members are to
   * be considered invalid.
   *
   * @return The aggregated traffic impact, or `std::nullopt` if the message has no events with traffic impact.
   */
  std::optional<TrafficImpact> GetTrafficImpact() const;

  /**
   * @brief Shifts timestamps to the present.
   *
   * This method is intended for testing. It shifts the timestamps of the message by a fixed amount,
   * so that `m_updateTime` corresponds to current time, and all other timestamps maintain their
   * offset to `m_updateTime`. If `m_startTime` and/or `m_endTime` are set, they may be adjusted
   * further to maintain their offset from midnight or the full hour (currently not implemented).
   */
  void ShiftTimestamps();

  std::string m_id;
  traffic::IsoTime m_receiveTime = traffic::IsoTime::Now();
  traffic::IsoTime m_updateTime = traffic::IsoTime::Now();
  traffic::IsoTime m_expirationTime = traffic::IsoTime::Now();
  std::optional<traffic::IsoTime> m_startTime = {};
  std::optional<traffic::IsoTime> m_endTime = {};
  bool m_cancellation = false;
  bool m_forecast = false;
  // TODO std::optional<Urgency> m_urgency;
  std::optional<TraffLocation> m_location;
  std::vector<TraffEvent> m_events;
  std::vector<std::string> m_replaces;
  GlobalSimpleColoring m_decoded;
};

using TraffFeed = std::vector<TraffMessage>;

// TODO Capabilities

/*
 * Filter: currently not implemented.
 * We only use bbox, for which we have a suitable data type.
 * min_road_class is not needed as we do not filter by road class.
 */

/*
 * TraffSubscription: currently not implemented.
 * We just store the ID as a string.
 * Filters are only by bbox, not by min_road_class. The list is auto-generated from the list of
 * active MWMs and changes exactly when the active MWM set changes, eliminating the need to store
 * the full filter list.
 */

/**
 * @brief Encapsulates the response to a TraFF request.
 */
struct TraffResponse
{
  /**
   * @brief The response status for the request which triggered the response.
   */
  ResponseStatus m_status = ResponseStatus::Invalid;

  /**
   * @brief The subscription ID which the source has assigned to the subscriber.
   *
   * This attribute is how the source communicates the subscription ID to a subscriber. Required for
   * responses to a subscription request; some transport channels may require it for every
   * subscription-related operation; forbidden otherwise.
   */
  std::string m_subscriptionId;

  /**
   * @brief The time in seconds after which the source will consider the subscription invalid if no
   * activity occurs.
   *
   * Required for responses to a subscription request on some transport channels, optional on other
   * channels, forbidden for other requests.
   *
   * If not used, the value is zero.
   */
  uint32_t m_timeout = 0;

  /**
   * @brief A feed of traffic messages sent as part of the response.
   */
  std::optional<TraffFeed> m_feed;
};

/**
 * @brief Merges coloring for one message into global coloring.
 *
 * After this function returns, `target` will hold the union of the entries it had prior to the
 * function call and the entries from `delta`.
 *
 * If the message has start/end times, they are mapped to `condition`, else `condition` is empty.
 * If `condition` is empty, entries from `delta` will be added to `target` as unconditional entries.
 * Otherwise, entries from `delta` will be added to `delta` with the condition specified by `condition`.
 * The condition will be created if it does not exist, else entries will be merged.
 *
 * In case of conflict, the more restrictive speed group wins. That is, `TempBlock` overrides
 * everything else, `Unknown` never overrides anything else, and among `G0` to `G5`, the lowest
 * group wins.
 *
 * @param delta Contains the entries to be added.
 * @param condition The condition for the entries in `delta`, empty for unconditional messages
 * @param target Receives the added entries.
 */
void MergeGlobalSimpleColoring(GlobalSimpleColoring const & delta,
                               std::optional<traffic::Condition> const & condition,
                               traffic::TrafficInfo::GlobalColoring & target);

std::string DebugPrint(Directionality directionality);
std::string DebugPrint(Ramps ramps);
std::string DebugPrint(RoadClass roadClass);
std::string DebugPrint(EventClass eventClass);
std::string DebugPrint(EventType eventType);
std::string DebugPrint(ResponseStatus status);
std::string DebugPrint(TrafficImpact impact);
std::string DebugPrint(Point const & point);
std::string DebugPrint(TraffLocation const & location);
std::string DebugPrint(TraffEvent const & event);
std::string DebugPrint(TraffMessage const & message);
std::string DebugPrint(TraffFeed const & feed);
}  // namespace traffxml
