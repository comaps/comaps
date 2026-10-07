#include "routing/segment.hpp"

#include "routing/fake_feature_ids.hpp"

#include <cstdint>
#include <sstream>

namespace routing
{
// Segment -----------------------------------------------------------------------------------------
uint32_t Segment::GetPointId(bool front) const
{
  return m_forward == front ? m_segmentIdx + 1 : m_segmentIdx;
}

bool Segment::operator<(Segment const & seg) const
{
  if (m_featureId != seg.m_featureId)
    return m_featureId < seg.m_featureId;

  if (m_segmentIdx != seg.m_segmentIdx)
    return m_segmentIdx < seg.m_segmentIdx;

  if (m_mwmId != seg.m_mwmId)
    return m_mwmId < seg.m_mwmId;

  return m_forward < seg.m_forward;
}

bool Segment::operator==(Segment const & seg) const
{
  return m_featureId == seg.m_featureId && m_segmentIdx == seg.m_segmentIdx && m_mwmId == seg.m_mwmId &&
         m_forward == seg.m_forward;
}

bool Segment::IsInverse(Segment const & seg) const
{
  return m_featureId == seg.m_featureId && m_segmentIdx == seg.m_segmentIdx && m_mwmId == seg.m_mwmId &&
         m_forward != seg.m_forward;
}

bool Segment::IsFakeCreated() const
{
  return m_featureId == FakeFeatureIds::kIndexGraphStarterId;
}

bool Segment::IsRealSegment() const
{
  return m_mwmId != kFakeNumMwmId && !FakeFeatureIds::IsTransitFeature(m_featureId);
}

// SegmentEdge -------------------------------------------------------------------------------------
bool SegmentEdge::operator==(SegmentEdge const & edge) const
{
  return m_target == edge.m_target && m_weight == edge.m_weight;
}

bool SegmentEdge::operator<(SegmentEdge const & edge) const
{
  if (m_target != edge.m_target)
    return m_target < edge.m_target;
  return m_weight < edge.m_weight;
}

std::string DebugPrint(Segment const & segment)
{
  std::ostringstream out;
  out << std::boolalpha << "Segment(" << segment.GetMwmId() << ", " << segment.GetFeatureId() << ", "
      << segment.GetSegmentIdx() << ", " << segment.IsForward() << ")";
  return out.str();
}

std::string DebugPrint(SegmentEdge const & edge)
{
  std::ostringstream out;
  out << "Edge(" << DebugPrint(edge.GetTarget()) << ", " << edge.GetWeight() << ")";
  return out.str();
}
}  // namespace routing

namespace std
{
size_t std::hash<routing::Segment>::operator()(routing::Segment const & segment) const
{
  uint64_t h = segment.GetFeatureId();
  h = h * 0x9E3779B97F4A7C15ULL + segment.GetSegmentIdx();
  h = h * 0x9E3779B97F4A7C15ULL + (uint64_t{segment.GetMwmId()} << 1) + (segment.IsForward() ? 1 : 0);
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdULL;
  h ^= h >> 29;
  return static_cast<size_t>(h);
}
}  // namespace std
