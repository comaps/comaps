#include "generator/road_penalty_generator.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/routing_helpers.hpp"

#include "routing/road_penalty_serialization.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/vehicle_mask.hpp"

#include "indexer/osm_element.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <optional>

namespace routing_builder
{
using namespace feature;
using namespace generator;
using namespace routing;
using std::string, std::vector;

namespace
{
// Unified penalty mapping for all OSM tags that create road penalties
std::map<OsmElement::Tag, RoadPenalty::Type> const kUnifiedPenaltyMapping = {
    // Traffic calming measures

    {OsmElement::Tag("traffic_calming", "rumble_strip"), RoadPenalty::Type::SmallCalming},
    {OsmElement::Tag("traffic_calming", "island"), RoadPenalty::Type::SmallCalming},

    {OsmElement::Tag("traffic_calming", "cushion"), RoadPenalty::Type::MediumCalming},
    {OsmElement::Tag("traffic_calming", "chicane"), RoadPenalty::Type::MediumCalming},
    {OsmElement::Tag("traffic_calming", "choker"), RoadPenalty::Type::MediumCalming},
    {OsmElement::Tag("traffic_calming", "table"), RoadPenalty::Type::MediumCalming},

    {OsmElement::Tag("traffic_calming", "bump"), RoadPenalty::Type::LargeCalming},
    {OsmElement::Tag("traffic_calming", "hump"), RoadPenalty::Type::LargeCalming},

    {OsmElement::Tag("crossing", "unmarked"), RoadPenalty::Type::LargeCalming},

    // Barrier penalties
    {OsmElement::Tag("barrier", "gate"), RoadPenalty::Type::Gate},
    {OsmElement::Tag("barrier", "lift_gate"), RoadPenalty::Type::Gate},
    {OsmElement::Tag("barrier", "swing_gate"), RoadPenalty::Type::Gate},
    {OsmElement::Tag("barrier", "toll_booth"), RoadPenalty::Type::Gate},

    // Junction penalties
    {OsmElement::Tag("highway", "stop"), RoadPenalty::Type::UncontrolledJunction},
    {OsmElement::Tag("highway", "give_way"), RoadPenalty::Type::UncontrolledJunction},
    {OsmElement::Tag("crossing", "uncontrolled"), RoadPenalty::Type::UncontrolledJunction},
    {OsmElement::Tag("crossing", "marked"), RoadPenalty::Type::UncontrolledJunction},
    // highway=crossing ommitted as often accompanied by traffic_signals

    {OsmElement::Tag("highway", "traffic_signals"), RoadPenalty::Type::ControlledJunction},
    {OsmElement::Tag("railway", "crossing"), RoadPenalty::Type::ControlledJunction},
    {OsmElement::Tag("railway", "level_crossing"), RoadPenalty::Type::ControlledJunction},
};

std::optional<RoadPenalty::Type> GetPenaltyByMapping(OsmElement const & elem)
{
  for (auto const & tag : elem.m_tags)
  {
    auto const it = kUnifiedPenaltyMapping.find(tag);
    if (it != kUnifiedPenaltyMapping.cend())
      return it->second;
  }
  return {};
}

void ReadRoadPenalty(std::string const & penaltyPath, routing::OsmWay2FeaturePoint & way2Feature,
                     RoadPenaltyByVehicleType & penalties)
{
  FileReader reader(penaltyPath);
  ReaderSource src(reader);

  // Read node penalties grouped by way
  uint64_t nodeWayCount = ReadPrimitiveFromSource<uint64_t>(src);
  while (nodeWayCount-- > 0)
  {
    uint64_t wayID = ReadPrimitiveFromSource<uint64_t>(src);
    uint32_t nodeCount = ReadPrimitiveFromSource<uint32_t>(src);

    for (uint32_t i = 0; i < nodeCount; ++i)
    {
      uint32_t nodeIdx = ReadPrimitiveFromSource<uint32_t>(src);
      m2::PointU coord;
      coord.x = ReadPrimitiveFromSource<uint32_t>(src);
      coord.y = ReadPrimitiveFromSource<uint32_t>(src);
      VehicleMask vehicleMask = ReadPrimitiveFromSource<VehicleMask>(src);

      // Read penalty type and derive time based on vehicle type
      RoadPenalty::Type type;
      routing::Load(src, type);

      // For each vehicle type, create penalty with vehicle-specific time
      for (uint8_t vType = 0; vType < static_cast<uint8_t>(VehicleType::Count); ++vType)
      {
        if (vehicleMask & (1 << vType))
        {
          // Create penalty with time specific to this vehicle type
          auto vehicleSpecificPenalty = RoadPenalty::Penalty(type, static_cast<VehicleType>(vType));

          // Process this location only once
          RoadPenalty::PointToPenalty pointToPenalty;
          way2Feature.ForEachNodeIdx(wayID, nodeIdx, coord, [&](uint32_t featureID, uint32_t nodeIdx)
          { pointToPenalty[RoadPoint(featureID, nodeIdx)] = vehicleSpecificPenalty; });

          if (!pointToPenalty.empty())
          {
            auto existingPenalties = penalties[vType].GetPointToPenalty();
            existingPenalties.insert(pointToPenalty.begin(), pointToPenalty.end());
            penalties[vType].SetPointPenalties(std::move(existingPenalties));
          }
        }
      }
    }
  }
}
}  // namespace

// RoadPenaltyCollector implementation
RoadPenaltyCollector::RoadPenaltyCollector(string const & filename, IDRInterfacePtr cache)
  : generator::CollectorInterface(filename)
  , m_cache(std::move(cache))
{
  // Empty - initialization handled in member initializer list
}

std::shared_ptr<CollectorInterface> RoadPenaltyCollector::Clone(IDRInterfacePtr const & cache) const
{
  return std::make_shared<RoadPenaltyCollector>(GetFilename(), cache ? cache : m_cache);
}

void RoadPenaltyCollector::CollectFeature(feature::FeatureBuilder const & fb, OsmElement const & elem)
{
  // Track roads for barrier processing
  if (elem.IsWay() && routing::IsCarRoad(fb.GetTypes()))
    m_roads.AddWay(elem);

  // Process only nodes for penalty types
  if (elem.IsNode())
  {
    auto penaltyType = GetPenaltyByMapping(elem);
    if (penaltyType)
      m_nodesWithType.Add(elem.m_id, ms::LatLon(elem.m_lat, elem.m_lon), *penaltyType);
  }
}

void RoadPenaltyCollector::MergeInto(RoadPenaltyCollector & collector) const
{
  m_nodesWithType.MergeInto(collector.m_nodesWithType);
  m_roads.MergeInto(collector.m_roads);
}

void RoadPenaltyCollector::Save()
{
  auto const fileName = GetFilename();
  LOG(LINFO, ("Saving road penalty values to", fileName));
  FileWriter writer(fileName);

  // All vehicles use the same penalty types

  // Group node penalties by way
  struct NodePenaltyEntry
  {
    uint32_t nodeIdx = 0;
    m2::PointU coord;
    RoadPenalty::Type type = RoadPenalty::Type::None;
  };

  std::map<uint64_t, std::vector<NodePenaltyEntry>> penaltiesByWay;

  m_roads.ForEachWayWithIndex([&](uint64_t wayID, std::vector<uint64_t> const & nodes, size_t idx)
  {
    std::vector<NodePenaltyEntry> wayNodePenalties;

    for (uint32_t nodeIdx = 0; nodeIdx < nodes.size(); ++nodeIdx)
    {
      uint64_t const nodeID = nodes[nodeIdx];
      auto const * barrierEntry = m_nodesWithType.Find(nodeID);
      if (barrierEntry == nullptr)
        continue;

      NodePenaltyEntry entry;
      entry.nodeIdx = nodeIdx;
      entry.coord = barrierEntry->m_coord;
      entry.type = barrierEntry->m_t;
      wayNodePenalties.push_back(entry);
    }

    if (!wayNodePenalties.empty())
      penaltiesByWay[wayID] = std::move(wayNodePenalties);
  }, m_cache);

  // Write node penalties grouped by way
  uint64_t const wayCount = penaltiesByWay.size();
  WriteToSink(writer, wayCount);
  for (auto const & [wayID, entries] : penaltiesByWay)
  {
    WriteToSink(writer, wayID);
    WriteToSink(writer, static_cast<uint32_t>(entries.size()));

    for (auto const & entry : entries)
    {
      WriteToSink(writer, entry.nodeIdx);
      WriteToSink(writer, entry.coord.x);
      WriteToSink(writer, entry.coord.y);
      WriteToSink(writer, kAllVehiclesMask);

      // Store only penalty type (time derived from vehicle type when loading)
      routing::Save(writer, entry.type);
    }
  }

  LOG(LINFO, ("Finished saving road penalty values"));
}

bool BuildRoadPenalty(string const & dataFilePath, string const & roadPenaltyPath,
                      routing::OsmWay2FeaturePoint & way2feature)
{
  LOG(LINFO, ("Generating road penalty info for", dataFilePath));

  try
  {
    RoadPenaltyByVehicleType penalties;
    ReadRoadPenalty(roadPenaltyPath, way2feature, penalties);

    FilesContainerW cont(dataFilePath, FileWriter::OP_WRITE_EXISTING);
    auto writer = cont.GetWriter(ROAD_PENALTY_FILE_TAG);

    // Write number of vehicle types
    uint32_t const vehicleTypeCount = penalties.size();
    WriteToSink(*writer, vehicleTypeCount);

    // Write penalty data for each vehicle type
    for (size_t i = 0; i < penalties.size(); ++i)
      RoadPenaltySerializer::Serialize(*writer, penalties[i]);

    return true;
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("No road penalty created:", ex.Msg()));
    return false;
  }
}
}  // namespace routing_builder
