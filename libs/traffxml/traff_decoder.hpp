#pragma once

#include "traffxml/traff_model.hpp"

#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"

// Only needed for OpenlrTraffDecoder, see below
#if 0
#include "openlr/openlr_decoder.hpp"
#include "openlr/openlr_model.hpp"
#endif

#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"
#include "routing/index_router.hpp"
#include "routing/regions_decl.hpp"
#include "routing/router.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "storage/country_info_getter.hpp"

#include <optional>

namespace traffxml
{
/**
 * @brief Abstract base class for all TraFF decoder implementations.
 *
 * At this point, `TraffDecoder` is single-threaded and not guaranteed to be thread-safe. This means
 * that all `TraffDecoder` operations should be limited to one thread or use appropriate thread
 * synchronization mechanisms. In particular, calling `DecodeMessage()` concurrently from multiple
 * threads is not supported.
 */
class TraffDecoder
{
public:
  using CountryInfoGetterFn = std::function<storage::CountryInfoGetter const &()>;
  using CountryParentNameGetterFn = std::function<std::string(std::string const &)>;

  TraffDecoder(DataSource & dataSource, CountryInfoGetterFn countryInfoGetter,
               const CountryParentNameGetterFn & countryParentNameGetter,
               std::map<std::string, traffxml::TraffMessage> & messageCache);

  virtual ~TraffDecoder() {}

  /**
   * @brief Decodes a single message to its segments and their speed groups.
   *
   * This method is not guaranteed to be thread-safe. All calls to this method should either be
   * strictly limited to one designated thread, or be synchronized using an appropriate mechanism.
   *
   * In addition to the above, this method may access the message cache which was passed to the
   * constructor. This is not thread-safe and needs to be synchronized, unless all other operations
   * on the message cache are guaranteed to happen on the same thread that called this method.
   *
   * @param message The message to decode.
   */
  void DecodeMessage(traffxml::TraffMessage & message);

protected:
  /**
   * @brief Decodes a TraFF location.
   *
   * @param message The message to decode.
   * @param decoded Receives the decoded segments. The speed group will be `Unknown`.
   */
  virtual void DecodeLocation(traffxml::TraffMessage & message, traffxml::MultiMwmColoring & decoded) = 0;

  /**
   * @brief Applies traffic impact to a decoded TraFF location.
   *
   * Applying impact sets the corresponding speed groups of the decoded segments. Existing speed groups will be overwritten.
   *
   * @param impact The traffic impact to apply.
   * @param decoded The decoded segments.
   */
  void ApplyTrafficImpact(traffxml::TrafficImpact & impact, traffxml::MultiMwmColoring & decoded);

  DataSource & m_dataSource;
  CountryInfoGetterFn m_countryInfoGetterFn;
  CountryParentNameGetterFn m_countryParentNameGetterFn;

  /**
   * @brief Cache of all currently active TraFF messages.
   *
   * Keys are message IDs, values are messages.
   */
  std::map<std::string, traffxml::TraffMessage> & m_messageCache;

  /**
   * @brief Consolidated traffic impact of the message currently being decoded
   */
  std::optional<traffxml::TrafficImpact> m_trafficImpact;

private:
};

// Disabled for now, as the OpenLR-based decoder is slow, buggy and not well suited to the task.
#if 0
/**
 * @brief A `TraffDecoder` implementation which internally uses the version 3 OpenLR decoder.
 */
class OpenLrV3TraffDecoder : public TraffDecoder
{
public:
  OpenLrV3TraffDecoder(DataSource & dataSource, CountryInfoGetterFn countryInfoGetter,
                       const CountryParentNameGetterFn & countryParentNameGetter,
                       std::map<std::string, traffxml::TraffMessage> & messageCache);

protected:
  /**
   * @brief Decodes a TraFF location.
   *
   * @param message The message to decode.
   * @param decoded Receives the decoded segments. The speed group will be `Unknown`.
   */
  void DecodeLocation(traffxml::TraffMessage & message, traffxml::MultiMwmColoring & decoded) override;

private:
  /**
   * @brief Returns the OpenLR functional road class (FRC) matching a TraFF road class.
   *
   * @param roadClass The TraFF road class.
   * @return The FRC.
   */
  static openlr::FunctionalRoadClass GetRoadClassFrc(std::optional<RoadClass> & roadClass);

  /**
   * @brief Guess the distance between two points.
   *
   * If both `p1` and `p2` have the `distance` attribute set, the difference between these two is
   * evaluated. If it is within a certain tolerance margin of the direct distance between the two
   * points, this value is returned. Otherwise, the distance is calculated from direct distance,
   * multiplied with a tolerance factor to account for the fact that the road is not always a
   * straight line.
   *
   * The result can be used to provide some semi-valid DNP values.
   *
   * @param p1 The first point.
   * @param p2 The second point.
   * @return The approximate distance on the ground, in meters.
   */
  static uint32_t GuessDnp(Point & p1, Point & p2);

  /**
   * @brief Converts a TraFF point to an OpenLR location reference point.
   *
   * Only coordinates are populated.
   *
   * @param point The point
   * @return An OpenLR LRP with the coordinates of the point.
   */
  static openlr::LocationReferencePoint PointToLrp(Point & point);

  /**
   * @brief Converts a TraFF location to an OpenLR linear location reference.
   *
   * @param location The location
   * @param backwards If true, gnerates a linear location reference for the backwards direction,
   * with the order of points reversed.
   * @return An OpenLR linear location reference which corresponds to the location.
   */
  static openlr::LinearLocationReference TraffLocationToLinearLocationReference(TraffLocation & location, bool backwards);

  /**
   * @brief Converts a TraFF location to a vector of OpenLR segments.
   *
   * Depending on the directionality, the resulting vector will hold one or two elements: one for
   * the forward direction, and for bidirectional locations, a second one for the backward
   * direction.
   *
   * @param location The location
   * @param messageId The message ID
   * @return A vector holding the resulting OpenLR segments.
   */
  static std::vector<openlr::LinearSegment> TraffLocationToOpenLrSegments(TraffLocation & location, std::string & messageId);

  /**
   * @brief The OpenLR decoder instance.
   *
   * Used to decode TraFF locations into road segments on the map.
   */
  openlr::OpenLRDecoder m_openLrDecoder;
};
#endif

/**
 * @brief A `TraffDecoder` implementation which internally uses the routing engine.
 */
class RoutingTraffDecoder : public TraffDecoder,
                            public MwmSet::Observer
{
public:
  class DecoderRouter : public routing::IndexRouter
  {
  public:
    /**
     * @brief Creates a new `DecoderRouter` instance.
     *
     * @param countryParentNameGetterFn Function which converts a country name into the name of its parent country)
     * @param countryFileFn Function which converts a pointer to its country name
     * @param countryRectFn Function which returns the rect for a country
     * @param numMwmIds
     * @param numMwmTree
     * @param trafficCache The traffic cache (used only if `vehicleType` is `VehicleType::Car`)
     * @param dataSource The MWM data source
     * @param decoder The `TraffDecoder` instance to which this router instance is coupled
     */
    DecoderRouter(CountryParentNameGetterFn const & countryParentNameGetterFn,
                  routing::TCountryFileFn const & countryFileFn,
                  routing::CountryRectFn const & countryRectFn,
                  std::shared_ptr<routing::NumMwmIds> numMwmIds,
                  std::unique_ptr<m4::Tree<routing::NumMwmId>> numMwmTree,
                  DataSource & dataSource, RoutingTraffDecoder & decoder);
  protected:
    /**
     * @brief Whether the set of fake endings generated for the check points is restricted.
     *
     * The return value is used internally when snapping checkpoints to edges. If this function
     * returns true, this instructs the `PointsOnEdgesSnapping` instance to consider only edges which
     * are not fenced off, i.e. can be reached from the respective checkpoint without crossing any
     * other edges. If it returns false, this restriction does not apply, and all nearby edges are
     * considered.
     *
     * Restricting the set of fake endings in this manner decreases the options considered for routing
     * and thus processing time, which is desirable for regular routing and has no side effects.
     * For TraFF location matching, simplification has undesirable side effects: if reference points
     * are located on one side of the road, the other carriageway may not be considered. This would
     * lead to situations like these:
     *
     * --<--<-+<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<+-<--<--
     * -->-->-+>==>==>==>==>==>==>-->-->-->-->-->-->-->-->-->-->-->==>==>==>==>==>==>==>==>+->-->--
     *                          *<                               <*
     *
     * (-- carriageway, + junction, < > direction, *< end point, <* start point, == route)
     *
     * To avoid this, the `DecoderRouter` implementation always returns false.
     */
    /**
     * @brief Returns the mode in which the router is operating.
     *
     * The `DecoderRouter` always returns `Mode::Decoding`.
     *
     * In navigation mode, the router may exit with `RouterResultCode::NeedMoreMaps` if it determines
     * that a better route can be calculated with additional maps. When snapping endpoints to edges,
     * it will consider only edges which are not “fenced off” by other edges, i.e. which can be
     * reached from the endpoint without crossing other edges. This decreases the number of fake
     * endings and thus speeds up routing, without any undesirable side effects for that use case.
     *
     * Asking the user to download extra maps is neither practical for a TraFF decoder which runs in
     * the background and may decode many locations, one by one, nor is it needed (if maps are
     * missing, we do not need to decode traffic reports for them).
     *
     * Eliminating fenced-off edges from the snapping candidates has an undesirable side effect for
     * TraFF location decoding on dual-carriageway roads: if the reference points are outside the
     * carriageways, only one direction gets considered for snapping, as the opposite direction is
     * fenced off by it. This may lead to situations like these:
     *
     * ~~~
     * --<--<-+<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<==<+-<--<--
     * -->-->-+>==>==>==>==>==>==>-->-->-->-->-->-->-->-->-->-->-->==>==>==>==>==>==>==>==>+->-->--
     *                          |<                               <|
     *
     * (-- carriageway, + junction, < > direction, |< end point, <| start point, == route)
     * ~~~
     *
     * Therefore, in decoding mode, the router will never exit with `RouterResultCode::NeedMoreMaps`
     * but tries to find a route with the existing maps, or exits without a route. When snapping
     * endpoints to edges, it considers all edges within the given radius, fenced off or not.
     */
    IndexRouter::Mode GetMode() override { return IndexRouter::Mode::Decoding; }

    /**
     * @brief Returns current routing options.
     *
     * For traffic decoding purposes, all roads are allowed.
     */
    routing::RoutingOptions GetRoutingOptions() override;

  private:
  };

  class TraffEstimator final : public routing::EdgeEstimator
  {
  public:
    TraffEstimator(DataSource * dataSourcePtr, std::shared_ptr<routing::NumMwmIds> numMwmIds,
                   double maxWeightSpeedKMpH,
                   routing::SpeedKMpH const & offroadSpeedKMpH,
                   RoutingTraffDecoder & decoder)
      : EdgeEstimator(routing::VehicleType::Car, maxWeightSpeedKMpH, offroadSpeedKMpH, dataSourcePtr, numMwmIds)
      , m_decoder(decoder)
    {
    }

    // EdgeEstimator overrides:

    /**
     * @brief Estimates travel time between two points along a direct fake edge.
     *
     * Estimates time in seconds it takes to go from point `from` to point `to` along direct fake edge.
     *
     * @param from The start point.
     * @param to The destination point.
     * @param purpose The purpose for which the result is to be used.
     * @return Travel time in seconds.
     */
    double CalcOffroad(ms::LatLon const & from, ms::LatLon const & to, Purpose /* purpose */) const override;
    double CalcSegmentWeight(routing::Segment const & segment, routing::RoadGeometry const & road, Purpose /* purpose */) const override;

    double GetUTurnPenalty(Purpose /* purpose */) const override;

    /**
     * @brief Determines the penalty factor for making a turn.
     *
     * The turn is at the first or last points of `from_road` and `to_road` and can be determined
     * by comparing the endpoints of `from_road` and `to_road` for a match.
     *
     * @param purpose The purpose for which the penalty is calculated, ignored by this implementation
     * @param angle The angle in degrees (negative values indicate a right turn)
     * @param from_road The road (segment between two junctions) before the turn
     * @param to_road The road (segment between two junctions) after the turn
     * @param is_left_hand_traffic True for left-hand traffic, false for right-hand traffic
     */
    double GetTurnPenalty(Purpose /* purpose */, double angle, routing::RoadGeometry const & from_road,
                          routing::RoadGeometry const & to_road, bool is_left_hand_traffic = false) const override;
    double GetFerryLandingPenalty(Purpose /* purpose */) const override;

    /**
     * @brief Whether access restrictions are ignored.
     *
     * A return value of false indicates that access restrictions should be observed, which is the
     * default behavior for a routing use case. If true, it indicates that routing should ignore
     * access restrictions. This is needed to resolve traffic message locations; it could also be
     * used e.g. for emergency vehicle use cases.
     *
     * This implementation may return true or false, depending on the location being decoded.
     */
    bool IsAccessIgnored() override;

  private:
    RoutingTraffDecoder & m_decoder;
  };

  struct JunctionCandidateInfo
  {
    JunctionCandidateInfo(double weight)
      : m_weight(weight)
    {}

    double m_weight;
    size_t m_segmentsIn = 0;
    size_t m_segmentsOut = 0;
    size_t m_twoWaySegments = 0;
  };

  RoutingTraffDecoder(DataSource & dataSource, CountryInfoGetterFn countryInfoGetter,
                      const CountryParentNameGetterFn & countryParentNameGetter,
                      std::map<std::string, traffxml::TraffMessage> & messageCache);

  /**
   * @brief Called when a map is registered for the first time and can be used.
   */
  void OnMapRegistered(platform::LocalCountryFile const & localFile) override;

  /**
   * @brief Called when a map is deregistered and can no longer be used.
   *
   * This implementation does nothing, as `NumMwmIds` does not support removal.
   */
  virtual void OnMapDeregistered(platform::LocalCountryFile const & /* localFile */) override {}

  /**
   * @brief Determines the penalty factor bases on how highway attributes match.
   *
   * This compares the highway type of the candidate feature (as retrieved from OSM) against the
   * road class and ramps attributes of the location.
   *
   * Rules are subject to change but principles are:
   *
   * Penalties for ramp mismatch and road class mismatch are applied consecutively, thus the maximum
   * penalty is `kAttributePenalty ^ 2`.
   *
   * If ramps mismatch (location specifies a ramp but candidate is not a ramp, or vice versa), the
   * penalty is `kAttributePenalty`.
   *
   * If road classes are similar, the penalty is `kReducedAttributePenalty`. For a complete
   * mismatch, the penalty is `kAttributePenalty`.
   *
   * @param highwayType The OSM highway type of the candidate feature.
   * @param roadClass The TraFF road class of the location.
   * @param ramps The ramps atribute of the TraFF location.
   *
   * @return 1 for a perfect match (same road class and ramp type), up to `kAttributePenalty ^ 2`
   * for a mismatch.
   */
  static double GetHighwayTypePenalty(std::optional<routing::HighwayType> highwayType,
                                      std::optional<RoadClass> roadClass,
                                      Ramps ramps);

  /**
   * @brief Determines the penalty factor based on how two reference numbers match.
   *
   * Rules are subject to change.
   *
   * This method takes a vector as an argument, compares each element and returns the penalty for
   * the best match.
   *
   * @param refs A vector of reference numbers of the current segment, compared against `m_roadRef`.
   *
   * @return 1 for a perfect match (refs are assumed to refer to the same object), `kAttributePenalty`
   * for a mismatch (refs are assumed to refer to different objects) or`kReducedAttributePenalty` for
   * a partial match (unclear whether both refs refer to the same object).
   */
  double GetRoadRefPenalty(std::vector<std::string> & refs) const;

  /**
   * @brief Determines the penalty factor based on how two reference numbers match.
   *
   * Rules are subject to change.
   *
   * @param ref The reference number of the current segment, compared against `m_roadRef`.
   *
   * @return 1 for a perfect match (refs are assumed to refer to the same object), `kAttributePenalty`
   * for a mismatch (refs are assumed to refer to different objects) or`kReducedAttributePenalty` for
   * a partial match (unclear whether both refs refer to the same object).
   */
  double GetRoadRefPenalty(std::string const & ref) const;

protected:
  /**
   * @brief Initializes the router.
   *
   * This is usually done in the constructor but fails if no maps are loaded (attempting to
   * construct a router without maps results in a crash, hence we check for maps and exit with an
   * error if we have none). It can be repeated any time.
   *
   * Attempting to initialize a router which has already been succesfully initialized is a no-op. It
   * will be reported as success.
   *
   * @return true if successful, false if not.
   */
  bool InitRouter();

  /**
   * @brief Adds a segment to the decoded segments.
   *
   * @param decoded The decoded segments.
   * @param segment The segment to add.
   */
  void AddDecodedSegment(traffxml::MultiMwmColoring & decoded, routing::Segment & segment);

  /**
   * @brief Decodes one direction of a TraFF location.
   *
   * @param message The message to decode.
   * @param decoded Receives the decoded segments. The speed group will be `Unknown`.
   * @param backwards If true, decode the backward direction, else the forward direction.
   */
  void DecodeLocationDirection(traffxml::TraffMessage & message,
                               traffxml::MultiMwmColoring & decoded, bool backwards);

  /**
   * @brief Decodes a TraFF location.
   *
   * @param message The message to decode.
   * @param decoded Receives the decoded segments. The speed group will be `Unknown`.
   */
  void DecodeLocation(traffxml::TraffMessage & message, traffxml::MultiMwmColoring & decoded) override;

  /**
   * @brief Truncates the route so its endpoints best match the reference points.
   *
   * Leading and trailing fake segments are discarded.
   *
   * When building the graph, the router creates fake segments to the nearest roads. These are not
   * necessarily the best for location decoding, which may result in “heads” or “tails” being added
   * to the decoded location. This function attempts to detect and remove them.
   *
   * To do this, it iterates over the nodes (taken from `rsegments`) and determines if any of them
   * is a better start/end candidate. This is done by calculating the cost of leaping between the
   * node and the corresponding checkpoint; if this is cheaper than the stretch of route bypassed
   * in this way, the node becomes a candidate for the corresponding endpoint. The higher the cost
   * saving, the better the candidate.
   *
   * After identifying the best candidate for each endpoint, segments outside these nodes are
   * discarded.
   *
   * @param rsegments The segments of the route
   * @param checkpoints The reference points (at least two)
   * @param backwards True when decoding the backward direction, false when decodign the forward direction.
   */
  void TruncateRoute(std::vector<routing::RouteSegment> & rsegments,
                     routing::Checkpoints const & checkpoints, bool backwards);

private:
  static void LogCode(routing::RouterResultCode code, double const elapsedSec);

  /**
   * @brief Populates the list of candidates for junction points.
   *
   * If the location has a fuzziness of `LowRes`, the map is searched for candidates around the
   * `from` and `to` points, which are taken from the `m_location` member of `m_message`. The weight
   * for each candidate is calculated based on its distance from the reference point and the match
   * between the attributes of the segment and the location. Since junction points are part of
   * multiple segments, the best match wins. Candidates and their weight are stored in
   * `m_startJunctions` and `m_endJunctions`.
   *
   * If the location’s fuzziness attribute is empty or does not equal `LowRes`, `m_startJunctions`
   * and `m_endJunctions` are cleared.
   */
  void GetJunctionPointCandidates();

  /**
   * @brief Populates a list of candidates for junction points.
   *
   * Implementation for `GetJunctionPointCandidates()`. The map is searched for candidates around
   * `point`. The weight for each candidate is calculated based on its distance from `point` and
   * the match between the attributes of the segment and the location of `m_message`. Since junction
   * points are part of multiple segments, the best match wins. Candidates and their weight are
   * stored in `junctions`.
   *
   * @param point The reference point
   * @param junctions Receives a list of junction candidates with their weight
   */
  void GetJunctionPointCandidates(Point const & point,
                                  std::map<m2::PointD, double> & junctions);

  /**
   * @brief Mutex for access to shared members.
   *
   * This is to prevent adding newly-registered maps while the router is in use.
   *
   * @todo As per the `MwmSet::Observer` documentation, implementations should be quick and lean,
   * as they may be called from any thread. Locking a mutex may be in conflict with this, as it may
   * mean locking up the caller while a location is being decoded.
   */
  std::mutex m_mutex;

  std::shared_ptr<routing::NumMwmIds> m_numMwmIds = std::make_shared<routing::NumMwmIds>();
  std::unique_ptr<routing::IRouter> m_router;
  std::optional<traffxml::TraffMessage> m_message = std::nullopt;

  /**
   * @brief Junction points near start of location, with their associated offroad weight.
   *
   * If the list is empty, no junction alignment at the `from` point will be done and decoding
   * relies solely on point coordinates.
   */
  std::map<m2::PointD, double> m_startJunctions;

  /**
   * @brief Junction points near end of location, with their associated offroad weight.
   *
   * If the list is empty, no junction alignment at the `to` point will be done and decoding
   * relies solely on point coordinates.
   */
  std::map<m2::PointD, double> m_endJunctions;

  /**
   * @brief Radius around reference points in which to search for junctions.
   *
   * Determined dynamically, based on distance between reference points.
   * Maximum distance is never more than half the distance between endpoints.
   * It should be between `kJunctionRadiusMin` and `kJunctionRadiusMax`, and as close as possible to
   * 1/3 the distance.
   */
  double m_junctionRadius;

  /**
   * @brief The road ref of `m_message`, parsed with `ParseRef()`
   */
  std::vector<std::string> m_roadRef;
};

/**
 * @brief The default TraFF decoder implementation, recommended for production use.
 */
//using DefaultTraffDecoder = OpenLrV3TraffDecoder;
using DefaultTraffDecoder = RoutingTraffDecoder;

traffxml::RoadClass GetRoadClass(routing::HighwayType highwayType);
double GetRoadClassPenalty(traffxml::RoadClass lhs, traffxml::RoadClass rhs);
bool IsRamp(routing::HighwayType highwayType);

/**
 * @brief Breaks down a ref into groups for comparison.
 *
 * The result of this function can be used to determine if two reference numbers match partially
 * (such as `A4`, `A4bis` and `A4.1`).
 *
 * Implementation details may change; currently the following applies:
 *
 * A whitespace character (or sequence of whitespace characters), or a switch between letters and
 * digits, starts a new group.
 *
 * Letters are converted to lowercase.
 *
 * For example, each of `A42`, `A 42` and `-a42` would be broken down into `a, 42`, whereas `A4.2`
 * would be broken down into `a, 4, 2`.
 */
std::vector<std::string> ParseRef(std::string const & ref);

/**
 * @brief Calculates the segments to truncate at the start of the route.
 *
 * The route is not actually truncated by this function.
 *
 * `start` and `startSaving` should be 0 when calling this function. After it returns, these values
 * will indicate the first segment to keep and the cost saved by truncating everything before.
 *
 * @param rsegments The segments of the route
 * @param checkpoints The reference points (at least two)
 * @param start Index of the first segment to keep
 * @param startSaving Cost saved by truncating
 * @param startWeight Weight of the fake segments up to the first real segment
 * @param junctions Junctions with the weight of their leap segment
 */
void TruncateStart(std::vector<routing::RouteSegment> & rsegments,
                   routing::Checkpoints const & checkpoints,
                   size_t & start, double & startSaving, double const startWeight,
                   std::map<m2::PointD, double> const & junctions);

/**
 * @brief Calculates the segments to truncate at the start of the route.
 *
 * The route is not actually truncated by this function.
 *
 * `end` should be `rsegments.size() - 1` and `endSaving` should be 0 when calling this function.
 * After it returns, these values will indicate the last segment to keep and the cost saved by
 * truncating everything after.
 *
 * @param rsegments The segments of the route
 * @param checkpoints The reference points (at least two)
 * @param end Index of the last segment to keep
 * @param endSaving Cost saved by truncating
 * @param endWeight Total weight of the route, including trailing fake segments
 * @param junctions Junctions with the weight of their leap segment
 */
void TruncateEnd(std::vector<routing::RouteSegment> & rsegments,
                 routing::Checkpoints const & checkpoints,
                 size_t & end, double & endSaving, double const endWeight,
                 std::map<m2::PointD, double> const & junctions);
}  // namespace traffxml
