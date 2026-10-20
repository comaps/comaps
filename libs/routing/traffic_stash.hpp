#pragma once

#include "traffic/speed_groups.hpp"
#include "traffic/traffic_info.hpp"

#include "routing_common/num_mwm_id.hpp"

#include <memory>
#include <optional>
#include <utility>

#include "3party/ankerl/unordered_dense.h"

namespace traffic
{
class TrafficCache;
}  // namespace traffic

namespace routing
{
class Segment;

class TrafficStash final
{
public:
  class Guard final
  {
  public:
    explicit Guard(std::shared_ptr<TrafficStash> stash) : m_stash(std::move(stash))
    {
      if (m_stash)
        m_stash->CopyTraffic();
    }

    ~Guard()
    {
      if (m_stash)
        m_stash->Clear();
    }

  private:
    std::shared_ptr<TrafficStash> m_stash;
  };

  TrafficStash(traffic::TrafficCache const & source, std::shared_ptr<NumMwmIds> numMwmIds);

  /**
   * @brief Returns the speed group for a segment.
   *
   * If `weight` is empty, only unconditional speed groups are evaluated. Otherwise, `weight` is the
   * travel time (in seconds) from the start of the route to the start of `segment`, and conditional
   * speed groups are evaluated if the condition is fulfilled `weight` seconds from now, with tolerance.
   *
   * @param segment The segment for which to retrieve the speed group.
   * @param weight travel time (in seconds) from start of route to start of `segment`, see description
   */
  traffic::SpeedGroup GetSpeedGroup(Segment const & segment, std::optional<double> weight) const;

  void SetColoring(NumMwmId numMwmId, std::shared_ptr<traffic::TrafficInfo::Coloring const> coloring);
  bool Has(NumMwmId numMwmId) const;

private:
  void CopyTraffic();

  void Clear() { m_mwmToTraffic.clear(); }

  traffic::TrafficCache const & m_source;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  ankerl::unordered_dense::map<NumMwmId, std::shared_ptr<traffic::TrafficInfo::Coloring const>> m_mwmToTraffic;
};
}  // namespace routing
