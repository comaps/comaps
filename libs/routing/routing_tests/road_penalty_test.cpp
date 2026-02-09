#include "testing/testing.hpp"

#include "routing/road_penalty.hpp"
#include "routing/road_point.hpp"
#include "routing/vehicle_mask.hpp"

#include <bit>
#include <optional>
#include <utility>

namespace road_penalty_test
{
using namespace routing;

UNIT_TEST(RoadPenalty_Basic)
{
  RoadPenalty penalty;

  // Test empty penalty
  TEST(!penalty.GetPenalty(RoadPoint(1, 0)).has_value(), ());
  TEST(!penalty.GetPenalty(RoadPoint(2, 1)).has_value(), ());
}

UNIT_TEST(RoadPenalty_PointPenalties)
{
  RoadPenalty penalty;

  // Test point penalties
  RoadPenalty::PointToPenalty pointPenalties;
  pointPenalties[RoadPoint(1, 0)] = RoadPenalty::Penalty(RoadPenalty::Type::MediumCalming, VehicleType::Car);
  pointPenalties[RoadPoint(2, 3)] = RoadPenalty::Penalty(RoadPenalty::Type::Gate, VehicleType::Car);
  penalty.SetPointPenalties(std::move(pointPenalties));

  auto p1 = penalty.GetPenalty(RoadPoint(1, 0));
  TEST(p1.has_value(), ());
  TEST_EQUAL(p1->m_type, RoadPenalty::Type::MediumCalming, ());
  TEST_EQUAL(p1->m_timeSeconds, 5, ());

  auto p2 = penalty.GetPenalty(RoadPoint(2, 3));
  TEST(p2.has_value(), ());
  TEST_EQUAL(p2->m_type, RoadPenalty::Type::Gate, ());
  TEST_EQUAL(p2->m_timeSeconds, 30, ());

  TEST(!penalty.GetPenalty(RoadPoint(1, 1)).has_value(), ());
}

UNIT_TEST(RoadPenalty_TypeConversion)
{
  // Test ToString/FromString
  TEST_EQUAL(ToString(RoadPenalty::Type::None), "None", ());
  TEST_EQUAL(ToString(RoadPenalty::Type::SmallCalming), "SmallCalming", ());
  TEST_EQUAL(ToString(RoadPenalty::Type::MediumCalming), "MediumCalming", ());
  TEST_EQUAL(ToString(RoadPenalty::Type::LargeCalming), "LargeCalming", ());
  TEST_EQUAL(ToString(RoadPenalty::Type::Gate), "Gate", ());
  TEST_EQUAL(ToString(RoadPenalty::Type::UncontrolledJunction), "UncontrolledJunction", ());
  TEST_EQUAL(ToString(RoadPenalty::Type::ControlledJunction), "ControlledJunction", ());

  RoadPenalty::Type type;
  FromString("SmallCalming", type);
  TEST_EQUAL(type, RoadPenalty::Type::SmallCalming, ());

  FromString("Gate", type);
  TEST_EQUAL(type, RoadPenalty::Type::Gate, ());

  FromString("UncontrolledJunction", type);
  TEST_EQUAL(type, RoadPenalty::Type::UncontrolledJunction, ());

  FromString("ControlledJunction", type);
  TEST_EQUAL(type, RoadPenalty::Type::ControlledJunction, ());
}

UNIT_TEST(RoadPenalty_Equality)
{
  RoadPenalty penalty1, penalty2;
  TEST(penalty1 == penalty2, ());

  RoadPenalty::PointToPenalty pointPenalties;
  pointPenalties[RoadPoint(1, 0)] = RoadPenalty::Penalty(RoadPenalty::Type::SmallCalming, VehicleType::Car);
  penalty1.SetPointPenalties(std::move(pointPenalties));

  TEST(!(penalty1 == penalty2), ());

  pointPenalties.clear();
  pointPenalties[RoadPoint(1, 0)] = RoadPenalty::Penalty(RoadPenalty::Type::SmallCalming, VehicleType::Car);
  penalty2.SetPointPenalties(std::move(pointPenalties));

  TEST(penalty1 == penalty2, ());
}

// Test vehicle mask functionality
UNIT_TEST(RoadPenalty_VehicleMask)
{
  // Test basic mask operations
  VehicleMask mask = 0;

  // Set bit for Car (VehicleType::Car = 2)
  mask |= (1 << 2);
  TEST((mask & (1 << 2)) != 0, ());
  TEST((mask & (1 << 1)) == 0, ());

  // Test mask with multiple vehicle types
  VehicleMask multiMask = 0;
  multiMask |= (1 << 0);  // Pedestrian
  multiMask |= (1 << 1);  // Bicycle
  multiMask |= (1 << 2);  // Car

  // Verify multiple bits are set
  TEST_EQUAL(std::popcount(multiMask), 3, ());
}

// Test default time penalties
UNIT_TEST(RoadPenalty_DefaultTimes)
{
  // Test GetTimePenalty with Car as default
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::None, VehicleType::Car), 0, ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::SmallCalming, VehicleType::Car), 3, ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::MediumCalming, VehicleType::Car), 5, ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::LargeCalming, VehicleType::Car), 7, ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::Gate, VehicleType::Car), 30, ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::UncontrolledJunction, VehicleType::Car), 15, ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::ControlledJunction, VehicleType::Car), 30, ());

  // Test constructor with vehicle type
  RoadPenalty::Penalty smallCalming(RoadPenalty::Type::SmallCalming, VehicleType::Car);
  TEST_EQUAL(smallCalming.m_type, RoadPenalty::Type::SmallCalming, ());
  TEST_EQUAL(smallCalming.m_timeSeconds, 3, ());

  RoadPenalty::Penalty gate(RoadPenalty::Type::Gate, VehicleType::Car);
  TEST_EQUAL(gate.m_type, RoadPenalty::Type::Gate, ());
  TEST_EQUAL(gate.m_timeSeconds, 30, ());
}

// Test vehicle-specific time penalties
UNIT_TEST(RoadPenalty_VehicleSpecificTimes)
{
  // Test GetTimePenalty with different vehicle types
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::Gate, VehicleType::Car), 30, ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::Gate, VehicleType::Bicycle), 10, ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::Gate, VehicleType::Pedestrian), 10, ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::Gate, VehicleType::Transit), 5, ());

  // Test constructor with vehicle type
  RoadPenalty::Penalty carGate(RoadPenalty::Type::Gate, VehicleType::Car);
  TEST_EQUAL(carGate.m_type, RoadPenalty::Type::Gate, ());
  TEST_EQUAL(carGate.m_timeSeconds, 30, ());

  RoadPenalty::Penalty bicycleGate(RoadPenalty::Type::Gate, VehicleType::Bicycle);
  TEST_EQUAL(bicycleGate.m_type, RoadPenalty::Type::Gate, ());
  TEST_EQUAL(bicycleGate.m_timeSeconds, 10, ());

  // Test traffic calming with different vehicles
  RoadPenalty::Penalty carCalming(RoadPenalty::Type::SmallCalming, VehicleType::Car);
  TEST_EQUAL(carCalming.m_type, RoadPenalty::Type::SmallCalming, ());
  TEST_EQUAL(carCalming.m_timeSeconds, 3, ());

  RoadPenalty::Penalty bicycleCalming(RoadPenalty::Type::SmallCalming, VehicleType::Bicycle);
  TEST_EQUAL(bicycleCalming.m_type, RoadPenalty::Type::SmallCalming, ());
  TEST_EQUAL(bicycleCalming.m_timeSeconds, 0, ());

  // Test junction penalties with different vehicles
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::UncontrolledJunction, VehicleType::Bicycle), 10,
             ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::ControlledJunction, VehicleType::Bicycle), 30, ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::UncontrolledJunction, VehicleType::Pedestrian), 0,
             ());
  TEST_EQUAL(RoadPenalty::Penalty::GetTimePenalty(RoadPenalty::Type::ControlledJunction, VehicleType::Pedestrian), 0,
             ());
}

}  // namespace road_penalty_test
