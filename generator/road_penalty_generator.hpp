#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/way_nodes_mapper.hpp"

#include "routing/road_penalty.hpp"
#include "routing/vehicle_mask.hpp"

#include "indexer/osm_element.hpp"

#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace routing
{
class OsmWay2FeaturePoint;

template <class Sink>
void Save(Sink & sink, RoadPenalty::Type const & type)
{
  WriteToSink(sink, static_cast<uint8_t>(type));
}

template <class Source>
void Load(Source & src, RoadPenalty::Type & type)
{
  uint8_t const res = ReadPrimitiveFromSource<uint8_t>(src);
  CHECK_LESS(res, static_cast<uint8_t>(RoadPenalty::Type::Count), ());
  type = static_cast<RoadPenalty::Type>(res);
}
}  // namespace routing

namespace routing_builder
{
using RoadPenalty = routing::RoadPenalty;
using VehicleType = routing::VehicleType;

using RoadPenaltyByVehicleType = std::array<RoadPenalty, static_cast<size_t>(VehicleType::Count)>;

class RoadPenaltyCollector : public generator::CollectorInterface
{
public:
  RoadPenaltyCollector(std::string const & filename, IDRInterfacePtr cache);

  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & cache = {}) const override;

  void CollectFeature(feature::FeatureBuilder const & fb, OsmElement const & elem) override;

  IMPLEMENT_COLLECTOR_IFACE(RoadPenaltyCollector);
  void MergeInto(RoadPenaltyCollector & collector) const;

protected:
  void Save() override;

private:
  IDRInterfacePtr m_cache;

  // Store only penalty types during collection phase
  generator::WayNodesMapper<RoadPenalty::Type> m_nodesWithType;
  generator::WaysIDHolder m_roads;
};

bool BuildRoadPenalty(std::string const & dataFilePath, std::string const & roadPenaltyPath,
                      routing::OsmWay2FeaturePoint & way2feature);
}  // namespace routing_builder
