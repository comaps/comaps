#pragma once

#include <cstdint>
#include <string>

namespace traffic
{
/**
 * @brief A bucket for the ratio of the speed of moving traffic to regular speed.
 *
 * Let Vmax be the regular speed (usually the posted speed limit) and Vreal the speed at which traffic
 * is currently flowing or expected to flow. The possible ratios (Vreal/Vmax) are grouped into buckets
 * and, from then on, only the bucket number is used.
 *
 * The threshold ratios for the individual values are defined in `kSpeedGroupThresholdPercentage`.
 *
 * Lower speed groups indicate lower speeds. When converting percentages to speed groups, boundaries
 * are defined as `(lower, upper]`. In the opposite direction, speed groups convert to their upper boundary.
 */
enum class SpeedGroup : uint8_t
{
  /** Traffic is moving (or expected to move) at no more than 8% of regular speed. */
  G0 = 0,
  /** Traffic is moving (or expected to move) at 8–16% of regular speed. */
  G1,
  /** Traffic is moving (or expected to move) at 16–33% of regular speed. */
  G2,
  /** Traffic is moving (or expected to move) at 33–58% of regular speed. */
  G3,
  /** Traffic is moving (or expected to move) at 58–83% of regular speed. */
  G4,
  /** Traffic is moving (or expected to move) at close to regular speed (more than 83%). */
  G5,
  /** Segment is impassable (closed or physically blocked). */
  TempBlock,
  /** Traffic situation (speed of moving traffic) is unknown. */
  Unknown,
  Count
};

static_assert(static_cast<uint8_t>(SpeedGroup::Count) <= 8, "");

/**
 * @brief Threshold ratios for the individual values of `SpeedGroup`.
 *
 * See `SpeedGroup` for how thresholds are used.
 *
 * `kSpeedGroupThresholdPercentage[g]` is the maximum percentage of Vreal/Vmax for group `g`.
 *
 * For special groups, where Vreal/Vmax is unknown or undefined, the threshold is 100%.
 */
extern uint32_t const kSpeedGroupThresholdPercentage[static_cast<size_t>(SpeedGroup::Count)];

/**
 * @brief Converts the ratio between speed of flowing traffic and the posted limit to a `SpeedGroup`.
 *
 * This method is used in traffic jam generation and performs the conversion from the ratio to a
 * `SpeedGroup` bucket. See `SpeedGroup` for how percentages map to speed groups.
 *
 * @param p Vreal / Vmax * 100% (ratio expressed in percent)
 *
 * @return the `SpeedGroup` value which corresponds to `p`
 */
SpeedGroup GetSpeedGroupByPercentage(double p);

/**
 * @brief Merges two speed groups.
 *
 * This function is intended where two sources specify two different speed groups for the same segment.
 * It will return the more restrictive of the two speed groups `lhs` and `rhs`: `SpeedGroup::TempBlock`
 * wins against everything else, `SpeedGroup::Unknown` loses against everything else; between two values
 * from `SpeedGroup::G0` and `SpeedGroup::G5`, the lower speed group wins.
 *
 * @param lhs See description
 * @param rhs See description
 *
 * @return the more restrictive of `lhs` and `rhs`
 */
SpeedGroup MergeSpeedGroups(SpeedGroup const & lhs, SpeedGroup const & rhs);

std::string DebugPrint(SpeedGroup const & group);
}  // namespace traffic
