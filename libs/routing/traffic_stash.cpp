#include "routing/traffic_stash.hpp"

#include "routing/segment.hpp"

#include "traffic/traffic_cache.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <map>

namespace routing
{
using namespace std;

TrafficStash::TrafficStash(traffic::TrafficCache const & source, shared_ptr<NumMwmIds> numMwmIds)
  : m_source(source)
  , m_numMwmIds(std::move(numMwmIds))
{
  CHECK(m_numMwmIds, ());
}

traffic::SpeedGroup TrafficStash::GetSpeedGroup(Segment const & segment, std::optional<double> weight) const
{
  auto res = traffic::SpeedGroup::Unknown;
  auto itMwm = m_mwmToTraffic.find(segment.GetMwmId());
  if (itMwm == m_mwmToTraffic.cend())
    return res;

  auto const & coloring = itMwm->second->m_unconditional;
  auto const itSeg = coloring.find(traffic::TrafficInfo::RoadSegmentId(
      segment.GetFeatureId(), base::asserted_cast<uint16_t>(segment.GetSegmentIdx()),
      segment.IsForward() ? traffic::TrafficInfo::RoadSegmentId::kForwardDirection
                          : traffic::TrafficInfo::RoadSegmentId::kReverseDirection));

  if (itSeg != coloring.cend())
    res = itSeg->second;

  if (weight)
  {
#if 0
    /*
     * Evaluate conditional traffic information within a defined range around ETA:
     * For the lower boundary, assume we are 10% faster than calculated.
     * For the upper boundary, assume:
     * - a fixed delay of 5 minutes (300 s)
     * - a delay of up to one hour during the first hour (100%, up to 3600 s)
     * - a delay uf up to one hour per additional four hours (25%)
     */
    auto minWeight = weight.value() * 0.9;
    auto maxWeight = (weight.value() <= 3600) ?
        (weight.value() * 2 + 300) :
        ((weight.value() - 3600) * 1.25 + 3600 * 2 + 300);
#else
    /*
     * Workaround: evaluate conditional traffic information within +/- 1 h of `weight`.
     * We get called indirectly from `IndexGraph::CalculateEdgeWeight()`, which passes not ETA, but
     * the weight (as used by A*), which may deviate significantly. See the comment in
     * `IndexGraph::CalculateEdgeWeight()` for a detailed discussion and solution approach.
     * Until then, we replicate the logic used for conditional access restrictions, which suffer from
     * the same issue, so at least we get consistent behavior between these two.
     */
    auto minWeight = weight.value() -3600;
    auto maxWeight = weight.value() + 3600;
#endif
    auto minTime = traffic::IsoTime::SecondsFromNow(minWeight);
    auto maxTime = traffic::IsoTime::SecondsFromNow(maxWeight);

    for (auto const & [condition, conditionalColoring] : itMwm->second->m_conditional)
      if (condition.IsTrueDuring(minTime, maxTime))
      {
        auto const itCondSeg = conditionalColoring.find(traffic::TrafficInfo::RoadSegmentId(
            segment.GetFeatureId(), base::asserted_cast<uint16_t>(segment.GetSegmentIdx()),
            segment.IsForward() ? traffic::TrafficInfo::RoadSegmentId::kForwardDirection
                : traffic::TrafficInfo::RoadSegmentId::kReverseDirection));
        if (itCondSeg != conditionalColoring.cend())
          res = traffic::MergeSpeedGroups(res, itCondSeg->second);
      }
  }

  return res;
}

void TrafficStash::SetColoring(NumMwmId numMwmId, shared_ptr<traffic::TrafficInfo::Coloring const> coloring)
{
  m_mwmToTraffic[numMwmId] = coloring;
}

bool TrafficStash::Has(NumMwmId numMwmId) const
{
  return m_mwmToTraffic.find(numMwmId) != m_mwmToTraffic.cend();
}

void TrafficStash::CopyTraffic()
{
  traffic::AllMwmTrafficInfo copy;
  m_source.CopyTraffic(copy);
  for (auto const & kv : copy)
  {
    auto const numMwmId = m_numMwmIds->GetId(kv.first.GetInfo()->GetLocalFile().GetCountryFile());
    CHECK(kv.second, ());
    SetColoring(numMwmId, kv.second);
  }
}
}  // namespace routing
