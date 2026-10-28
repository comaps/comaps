#pragma once

#include <cstdint>
#include <string>

namespace traffic
{
/**
 * A bucket for the ratio of the speed of moving traffic to the posted speed limit.
 *
 * Let Vmax be the posted speed limit and Vreal the speed at which traffic is currently flowing
 * or expected to flow. The possible ratios (Vreal/Vmax) are grouped into buckets and, from then
 * on, only the bucket number is used.
 *
 * The threshold ratios for the individual values are defined in `kSpeedGroupThresholdPercentage`.
 */
enum class SpeedGroup : uint8_t
{
  G0 = 0,
  G1,
  G2,
  G3,
  G4,
  G5,
  TempBlock,
  Unknown,
  Count
};

static_assert(static_cast<uint8_t>(SpeedGroup::Count) <= 8, "");

/**
 * Threshold ratios for the individual values of `SpeedGroup`.
 *
 * Let Vmax be the posted speed limit and Vreal the speed at which traffic is currently flowing
 * or expected to flow. The possible ratios (Vreal/Vmax) are grouped into buckets and, from then
 * on, only the bucket number is used.
 *
 * `kSpeedGroupThresholdPercentage[g]` is the maximum percentage of Vreal/Vmax for group g. Values
 * falling on the border of two groups may belong to either group.
 *
 * For special groups, where Vreal/Vmax is unknown or undefined, the threshold is 100%.
 */
extern uint32_t const kSpeedGroupThresholdPercentage[static_cast<size_t>(SpeedGroup::Count)];

/**
 * Converts the ratio between speed of flowing traffic and the posted limit to a `SpeedGroup`.
 *
 * This method is used in traffic jam generation: Let Vmax be the posted speed limit and Vreal the
 * speed at which traffic is currently flowing or expected to flow. The possible ratios
 * (Vreal/Vmax) are grouped into buckets and, from then on, only the bucket number is used.
 *
 * This method performs the conversion from the ratio to a `SpeedGroup` bucket.
 *
 * @param p Vreal / Vmax * 100% (ratio expressed in percent)
 *
 * @return the `SpeedGroup` value which corresponds to `p`
 */
SpeedGroup GetSpeedGroupByPercentage(double p);

std::string DebugPrint(SpeedGroup const & group);
}  // namespace traffic
