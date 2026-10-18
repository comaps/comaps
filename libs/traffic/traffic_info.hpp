#pragma once

#include "traffic/iso_time.hpp"
#include "traffic/speed_groups.hpp"

#include "indexer/mwm_set.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace platform
{
class HttpClient;
}

namespace traffic
{
/**
 * @brief Represents a condition for a traffic situation.
 *
 * An unconditional traffic situation is assumed to be active from the moment it is reported until
 * it expires. Conditions can be used to constrain this time, e.g. for planned events with a known
 * start and/or end time.
 *
 * A condition currently consists of a start and end time; the traffic situation is assumed to be active
 * during this entire period. If start time is empty, the situation is assumed to be active from the
 * moment it is reported. If end time is empty, the situation is assumed to be active indefinitely,
 * as long as it has not expired.
 *
 * Future versions may add schedules, e.g. to indicate that a situation is active only during certain
 * hours of the day.
 *
 * Comparison is rudimentary and only for the purpose of using `Condition` as a map key. It is implemented
 * by comparing start times first, and then end times if start times are considered equal. Empty values
 * are considered equal. An empty start time is considered less (earlier) than an explicit one, an empty
 * end time is considered more (later) than an explicit one.
 */
class Condition
{
public:
  std::optional<IsoTime> m_startTime = {};
  std::optional<IsoTime> m_endTime = {};
  bool operator< (const Condition & rhs) const;
};

/**
 * @brief The `TrafficInfo` class is responsible for providing the real-time information about road
 * traffic for one MWM.
 */
class TrafficInfo
{
public:
  static uint8_t const kLatestKeysVersion;
  static uint8_t const kLatestValuesVersion;

  /**
   * @brief Whether traffic data is available in this `TrafficInfo` instance.
   */
  /*
   * TODO A global traffic update would require some 2–3 states:
   * * IsAvailable
   * * Data available but not yet decoded
   * * (possibly) No traffic reports for this MWM
   */
  enum class Availability
  {
    /** This `TrafficInfo` instance has data available. */
    IsAvailable,
    /** No traffic data is available (file not found on the server, or server returned invalid data). */
    NoData,
    /** Traffic data could not be retrieved because the map data is outdated. */
    ExpiredData,
    /** Traffic data could not be retrieved because the app version is outdated. */
    ExpiredApp,
    /** No traffic data is available because the server responded with an error (other than “not found”), or no request was made yet. */
    Unknown
  };

  /**
   * @brief The RoadSegmentId struct models a segment of a road.
   *
   * A road segment is the link between two consecutive points of an OSM way. The way must be
   * tagged with a valid `highway` tag. A segment refers to a single direction.
   *
   * Therefore, an OSM way with `n` points has `n - 1` segments if tagged as one-way, `2 (n - 1)`
   * otherwise (as each pair of adjacent points is connected by two segments, one in each
   * direction.)
   */
  struct RoadSegmentId
  {
    // m_dir can be kForwardDirection or kReverseDirection.
    static uint8_t constexpr kForwardDirection = 0;
    static uint8_t constexpr kReverseDirection = 1;

    RoadSegmentId();

    RoadSegmentId(uint32_t fid, uint16_t idx, uint8_t dir);

    bool operator==(RoadSegmentId const & o) const { return m_fid == o.m_fid && m_idx == o.m_idx && m_dir == o.m_dir; }

    bool operator<(RoadSegmentId const & o) const
    {
      if (m_fid != o.m_fid)
        return m_fid < o.m_fid;
      if (m_idx != o.m_idx)
        return m_idx < o.m_idx;
      return m_dir < o.m_dir;
    }

    uint32_t GetFid() const { return m_fid; }
    uint16_t GetIdx() const { return m_idx; }
    uint8_t GetDir() const { return m_dir; }

    // The ordinal number of feature this segment belongs to.
    uint32_t m_fid;

    // The ordinal number of this segment in the list of
    // its feature's segments.
    uint16_t m_idx : 15;

    // The direction of the segment.
    uint8_t m_dir : 1;
  };

  /**
   * @brief Mapping from feature segments to speed groups (see `speed_groups.hpp`), condition-agnostic and for one MWM.
   *
   * `SimpleColoring` does not have a representation for conditions (time windows) in which the mapping
   * applies. Is this functionality is needed, use `ConditionalColoring` or `Coloring`.
   */
  // todo(@m) unordered_map?
  using SimpleColoring = std::map<RoadSegmentId, SpeedGroup>;

  /**
   * @brief Conditional mapping from feature segments to speed groups (see `speed_groups.hpp`), for one MWM.
   *
   * Each entry has a time slot during which it is valid, expressed as `Condition`, and the mapping
   * that applies during that time slot.
   *
   * Entries may overlap in time and contain different sets of segments. To determine the speed group
   * of a particular segment at a given time, it is therefore necessary to evaluate every matching
   * entry which has a mapping for the segment and resolve conflicting speed group information.
   *
   * By convention, the most restrictive speed group wins: `TempBlock` wins over anything else, `Unknown`
   * loses against anything else; between different speeds, the lowest wins.
   */
  using ConditionalColoring = std::map<Condition, SimpleColoring>;

  /**
   * @brief Mapping from feature segments to speed groups (see `speed_groups.hpp`), for one MWM.
   *
   * `Coloring` represents both conditional and unconditional mappings.
   *
   * To determine the speed group of a particular segment at a given time, it is necessary to evaluate
   * both `m_unconditional` and every matching entry from `m_conditional`, gather all mappings for the
   * segment in question and resolve conflicting speed group information.
   *
   * By convention, the most restrictive speed group wins: `TempBlock` wins over anything else, `Unknown`
   * loses against anything else; between different speeds, the lowest wins.
   */
  struct Coloring
  {
    /**
     * @brief Unconditional coloring.
     *
     * These mappings apply at all times, unless overruled by a more restrictive speed group in
     * `m_conditional`.
     */
    SimpleColoring m_unconditional;

    /**
     * @brief Conditional coloring.
     *
     * These mappings apply during the time specified, unless overruled by another, more restrictive
     * speed group in the same map or in `m_unconditional`.
     */
    ConditionalColoring m_conditional;
  };

  /**
   * @brief Mapping from feature segments to speed groups (see `speed_groups.hpp`), for all MWMs.
   *
   * `GlobalColoring` represents both conditional and unconditional mappings.
   */
  using GlobalColoring = std::map<MwmSet::MwmId, Coloring>;

  TrafficInfo() = default;

  TrafficInfo(MwmSet::MwmId const & mwmId, Coloring && coloring);

  /**
   * @brief Returns a `TrafficInfo` instance with pre-populated traffic information.
   * @param coloring The traffic information (road segments and their speed group)
   * @return The new `TrafficInfo` instance
   */
  static TrafficInfo BuildForTesting(Coloring && coloring);
  void SetTrafficKeysForTesting(std::vector<RoadSegmentId> const & keys);

  /**
   * @brief Returns the latest known speed group by a feature segment's ID.
   * @param id The road segment ID.
   * @return The speed group, or `SpeedGroup::Unknown` if no information is available.
   */
  SpeedGroup GetSpeedGroup(RoadSegmentId const & id) const;

  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }
  Coloring const & GetColoring() const { return m_coloring; }
  Availability GetAvailability() const { return m_availability; }

#if 0
  /*
   * These were formerly used by pytraffic.
   * Commented out as the whole construct no longer works as-is with the introduction of conditional
   * traffic coloring.
   */
  /**
   * @brief Extracts RoadSegmentIds from an MWM and stores them in a sorted order.
   * @param mwmPath Path to the MWM file
   *
   * @todo We don’t need this any longer as the API has been reworked: We no longer separate keys
   * (segment IDs) from values (their speed groups) and no longer have a use case for retrieving a
   * list of all possible segment IDs – rather, we decode TraFF messages into segments, or have a
   * cached list of the segments affected by a particular message. However, pytraffic still has some
   * references to this function. We need to clean those up first, then we can delete this function.
   */
  static void ExtractTrafficKeys(std::string const & mwmPath, std::vector<RoadSegmentId> & result);

  /**
   * @brief Adds unknown values to a partially known coloring map.
   *
   * After this method returns, the keys of `result` will be exactly `keys`. The speed group
   * associated with each key will be the same as in `knownColors`, or `SpeedGroup::Unknown` for
   * keys which are not found in `knownColors`.
   *
   * Keys in `knownColors` which are not in `keys` will be ignored.
   *
   * If `result` contains mappings prior to this method being called, they will be deleted.
   *
   * @param keys The keys for the result map.
   * @param knownColors The map containing the updates.
   * @param result The map to be updated.
   */
  static void CombineColorings(std::vector<TrafficInfo::RoadSegmentId> const & keys,
                               TrafficInfo::Coloring const & knownColors, TrafficInfo::Coloring & result);

  // Serializes the keys of the coloring map to |result|.
  // The keys are road segments ids which do not change during
  // an mwm's lifetime so there's no point in downloading them every time.
  /*
   * TODO We don’t need these any longer as the format is obsolete, but pytraffic still has some
   * references to these. We need to clean those up first, then we can delete these functions.
   */
  static void DeserializeTrafficKeys(std::vector<uint8_t> const & data, std::vector<RoadSegmentId> & result);

  static void SerializeTrafficValues(std::vector<SpeedGroup> const & values, std::vector<uint8_t> & result);
#endif

private:
  /**
   * @brief Result of the last request to the server.
   */
  enum class ServerDataStatus
  {
    /** New data was returned. */
    New,
    /** Data has not changed since the last request. */
    NotChanged,
    /** The URL was not found on the server. */
    NotFound,
    /** An error prevented data from being requested, or the server responded with an error. */
    Error,
  };

  /**
   * @brief The mapping from feature segments to speed groups (see speed_groups.hpp).
   */
  Coloring m_coloring;

  /**
   * @brief The keys of the coloring map. The values are downloaded periodically
   * and combined with the keys to form `m_coloring`.
   * *NOTE* The values must be received in the exact same order that the keys are saved in.
   */
  std::vector<RoadSegmentId> m_keys;

  MwmSet::MwmId m_mwmId;
  Availability m_availability = Availability::Unknown;
  int64_t m_currentDataVersion = 0;
};

class TrafficObserver
{
public:
  virtual ~TrafficObserver() = default;

  virtual void OnTrafficInfoClear() = 0;
  virtual void OnTrafficInfoAdded(traffic::TrafficInfo && info) = 0;
  virtual void OnTrafficInfoRemoved(MwmSet::MwmId const & mwmId) = 0;
};

std::string DebugPrint(TrafficInfo::RoadSegmentId const & id);
}  // namespace traffic
