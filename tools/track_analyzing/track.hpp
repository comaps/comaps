#pragma once

#include "routing/segment.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "coding/traffic.hpp"

#include <string>
#include <vector>

#include "3party/ankerl/unordered_dense.h"

namespace track_analyzing
{
using DataPoint = coding::TrafficGPSEncoder::DataPoint;
using Track = std::vector<DataPoint>;
using UserToTrack = ankerl::unordered_dense::map<std::string, Track>;
using MwmToTracks = ankerl::unordered_dense::map<routing::NumMwmId, UserToTrack>;

class MatchedTrackPoint final
{
public:
  MatchedTrackPoint(DataPoint const & dataPoint, routing::Segment const & segment)
    : m_dataPoint(dataPoint)
    , m_segment(segment)
  {}

  DataPoint const & GetDataPoint() const { return m_dataPoint; }
  routing::Segment const & GetSegment() const { return m_segment; }

private:
  DataPoint const m_dataPoint;
  routing::Segment const m_segment;
};

using MatchedTrack = std::vector<MatchedTrackPoint>;
using UserToMatchedTracks = ankerl::unordered_dense::map<std::string, std::vector<MatchedTrack>>;
using MwmToMatchedTracks = ankerl::unordered_dense::map<routing::NumMwmId, UserToMatchedTracks>;

class TrackFilter final
{
public:
  TrackFilter(uint64_t minDuration, double minLength, double minSpeed, double maxSpeed, bool ignoreTraffic);

  bool Passes(uint64_t duration, double length, double speed, bool hasTrafficPoints) const;

private:
  uint64_t const m_minDuration;
  double const m_minLength;
  double const m_minSpeed;
  double const m_maxSpeed;
  bool const m_ignoreTraffic;
};
}  // namespace track_analyzing
