#pragma once

#include "traffic/speed_groups.hpp"

#include "indexer/mwm_set.hpp"

#include <cstdint>
#include <map>
#include <vector>

namespace platform
{
class HttpClient;
}

namespace traffic
{
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
   * @brief Mapping from feature segments to speed groups (see `speed_groups.hpp`), for one MWM.
   */
  // todo(@m) unordered_map?
  using Coloring = std::map<RoadSegmentId, SpeedGroup>;

  TrafficInfo() = default;

  TrafficInfo(MwmSet::MwmId const & mwmId, int64_t currentDataVersion);

  /**
   * @brief Returns a `TrafficInfo` instance with pre-populated traffic information.
   * @param coloring The traffic information (road segments and their speed group)
   * @return The new `TrafficInfo` instance
   */
  static TrafficInfo BuildForTesting(Coloring && coloring);
  void SetTrafficKeysForTesting(std::vector<RoadSegmentId> const & keys);

  // Fetches the latest traffic data from the server and updates the coloring and ETag.
  // Construct the url by passing an MwmId.
  // The ETag or entity tag is part of HTTP, the protocol for the World Wide Web.
  // It is one of several mechanisms that HTTP provides for web cache validation,
  // which allows a client to make conditional requests.
  // *NOTE* This method must not be called on the UI thread.
  bool ReceiveTrafficData(std::string & etag);

  /**
   * @brief Returns the latest known speed group by a feature segment's ID.
   * @param id The road segment ID.
   * @return The speed group, or `SpeedGroup::Unknown` if no information is available.
   */
  SpeedGroup GetSpeedGroup(RoadSegmentId const & id) const;

  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }
  Coloring const & GetColoring() const { return m_coloring; }
  Availability GetAvailability() const { return m_availability; }

  /**
   * @brief Extracts RoadSegmentIds from an MWM and stores them in a sorted order.
   * @param mwmPath Path to the MWM file
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
  // todo(@m) Document the format.
  static void SerializeTrafficKeys(std::vector<RoadSegmentId> const & keys, std::vector<uint8_t> & result);

  static void DeserializeTrafficKeys(std::vector<uint8_t> const & data, std::vector<RoadSegmentId> & result);

  static void SerializeTrafficValues(std::vector<SpeedGroup> const & values, std::vector<uint8_t> & result);

  static void DeserializeTrafficValues(std::vector<uint8_t> const & data, std::vector<SpeedGroup> & result);

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

  friend void UnitTest_TrafficInfo_UpdateTrafficData();

  // todo(@m) A temporary method. Remove it once the keys are added
  // to the generator and the data is regenerated.
  bool ReceiveTrafficKeys();

  // Tries to read the values of the Coloring map from server into |values|.
  // Returns result of communicating with server as ServerDataStatus.
  // Otherwise, returns false and does not change m_coloring.
  ServerDataStatus ReceiveTrafficValues(std::string & etag, std::vector<SpeedGroup> & values);

  // Updates the coloring and changes the availability status if needed.
  bool UpdateTrafficData(std::vector<SpeedGroup> const & values);

  ServerDataStatus ProcessFailure(platform::HttpClient const & request, int64_t const mwmVersion);

  /** @brief The mapping from feature segments to speed groups (see speed_groups.hpp). */
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
