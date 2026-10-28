#pragma once

#include "routing/data_source.hpp"
#include "routing/features_road_graph.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/car_model.hpp"

#include "indexer/feature_data.hpp"

#include "geometry/point2d.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace openlr
{
// TODO(mgsergio): Inherit from FeaturesRoadGraph.
class Graph
{
public:
  using Edge = routing::Edge;
  using EdgeListT = routing::FeaturesRoadGraph::EdgeListT;
  using EdgeVector = routing::FeaturesRoadGraph::EdgeVector;
  using Junction = geometry::PointWithAltitude;

  Graph(DataSource & dataSource, std::shared_ptr<routing::CarModelFactory> carModelFactory);

  /**
   * Appends edges to `edges` such as that `edge.GetStartJunction() == junction`.
   */
  void GetOutgoingEdges(geometry::PointWithAltitude const & junction, EdgeListT & edges);

  /**
   * Appends edges to `edges` such as that `edge.GetEndJunction() == junction`.
   */
  void GetIngoingEdges(geometry::PointWithAltitude const & junction, EdgeListT & edges);

  /**
   * Appends edges to `edges` such as that `edge.GetStartJunction() == junction` and
   * `edge.IsFake() == false`.
   */
  void GetRegularOutgoingEdges(Junction const & junction, EdgeListT & edges);

  /**
   * Appends edges to `edges` such as that `edge.GetEndJunction() == junction` and
   * `edge.IsFake() == false`.
   */
  void GetRegularIngoingEdges(Junction const & junction, EdgeListT & edges);

  void FindClosestEdges(m2::PointD const & point, uint32_t const count,
                        std::vector<std::pair<Edge, Junction>> & vicinities) const;

  void AddIngoingFakeEdge(Edge const & e);
  void AddOutgoingFakeEdge(Edge const & e);

  void ResetFakes() { m_graph.ResetFakes(); }

  void GetFeatureTypes(FeatureID const & featureId, feature::TypesHolder & types) const;

  using EdgeCacheT = std::map<Junction, EdgeListT>;

private:
  routing::MwmDataSource m_dataSource;
  routing::FeaturesRoadGraph m_graph;
  EdgeCacheT m_outgoingCache, m_ingoingCache;
};
}  // namespace openlr
