#pragma once

#include "routing/directions_engine_helpers.hpp"
#include "routing/index_road_graph.hpp"
#include "routing/loaded_path_segment.hpp"
#include "routing/road_graph.hpp"
#include "routing/vehicle_mask.hpp"

#include "indexer/feature.hpp"

#include <memory>
#include <vector>

namespace routing
{
namespace turns
{
class IRoutingResult;
struct TurnItem;
}  // namespace turns

enum class RouterResultCode;
class MwmDataSource;

class DirectionsEngine
{
public:
  DirectionsEngine(MwmDataSource & dataSource, std::shared_ptr<NumMwmIds> numMwmIds);
  virtual ~DirectionsEngine() = default;

  // @TODO(bykoianko) Method Generate() should fill
  // vector<RouteSegment> instead of corresponding arguments.
  /**
   * @brief Calculates segments from a path on a route graph.
   *
   * Segments are calculated from `graph` (the route graph) and `path` (points on the route); each
   * pair of consecutive points becomes a segment.
   *
   * @param graph The route graph
   * @param path The route path, an ordered list of points on the route
   * @param cancellable
   * @param routeSegments Receives the list of segments
   * @return true on successful completion, false if cancelled or an error occurred
   */
  bool Generate(IndexRoadGraph const & graph, std::vector<geometry::PointWithAltitude> const & path,
                base::Cancellable const & cancellable, std::vector<RouteSegment> & routeSegments);
  void Clear();

  void SetVehicleType(VehicleType const & vehicleType) { m_vehicleType = vehicleType; }

protected:
  /*!
   * \brief GetTurnDirection makes a primary decision about turns on the route.
   * \param outgoingSegmentIndex index of an outgoing segments in vector result.GetSegments().
   * \param turn is used for keeping the result of turn calculation.
   */
  virtual size_t GetTurnDirection(turns::IRoutingResult const & result, size_t const outgoingSegmentIndex,
                                  NumMwmIds const & numMwmIds, RoutingSettings const & vehicleSettings,
                                  turns::TurnItem & turn) = 0;
  virtual void FixupTurns(std::vector<RouteSegment> & routeSegments) = 0;
  std::unique_ptr<FeatureType> GetFeature(FeatureID const & featureId);
  void LoadPathAttributes(FeatureID const & featureId, LoadedPathSegment & pathSegment, bool isForward);
  void GetSegmentRangeAndAdjacentEdges(IRoadGraph::EdgeListT const & outgoingEdges, Edge const & inEdge,
                                       uint32_t startSegId, uint32_t endSegId, SegmentRange & segmentRange,
                                       turns::TurnCandidates & outgoingTurns);
  /// \brief The method gathers sequence of segments according to IsJoint() method
  /// and fills |m_adjacentEdges| and |m_pathSegments|.
  void FillPathSegmentsAndAdjacentEdgesMap(IndexRoadGraph const & graph,
                                           std::vector<geometry::PointWithAltitude> const & path,
                                           IRoadGraph::EdgeVector const & routeEdges,
                                           base::Cancellable const & cancellable);

  AdjacentEdgesMap m_adjacentEdges;
  TUnpackedPathSegments m_pathSegments;

  MwmDataSource & m_dataSource;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  VehicleType m_vehicleType = VehicleType::Count;

private:
  void MakeTurnAnnotation(IndexRoadGraph::EdgeVector const & routeEdges, std::vector<RouteSegment> & routeSegments);

  ftypes::IsLinkChecker const & m_linkChecker;
  ftypes::IsRoundAboutChecker const & m_roundAboutChecker;
  ftypes::IsOneWayChecker const & m_onewayChecker;
};
}  // namespace routing
