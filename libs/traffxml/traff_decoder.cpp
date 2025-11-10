#include "traffxml/traff_decoder.hpp"

//#include "traffxml/traff_foo.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "indexer/feature.hpp"
#include "indexer/road_shields_parser.hpp"

// Only needed for OpenlrTraffDecoder, see below
#if 0
#include "openlr/decoded_path.hpp"
#include "openlr/openlr_decoder.hpp"
#include "openlr/openlr_model.hpp"
#endif

#include "routing/async_router.hpp"
#include "routing/checkpoints.hpp"
#include "routing/edge_estimator.hpp"
#include "routing/maxspeeds.hpp"
#include "routing/route.hpp"
#include "routing/router_delegate.hpp"
#include "routing/routing_helpers.hpp"

#include "routing_common/car_model.hpp"
#include "routing_common/maxspeed_conversion.hpp"

#include "storage/routing_helpers.hpp"

#include "traffic/traffic_cache.hpp"

#include <boost/algorithm/string.hpp>

namespace traffxml
{
enum class RefParserState
{
  Whitespace,
  Alpha,
  Numeric
};

// Only needed for OpenlrTraffDecoder, see below
#if 0
// Number of worker threads for the OpenLR decoder
/*
 * TODO how to determine the best number of worker threads?
 * One per direction? Does not seem to help with bidirectional locations (two reference points).
 * One per segment (from–via/from–at, via–to/at–to)? Not yet tested.
 * Otherwise there is little to be gained, as we decode messages one at a time.
 */
auto constexpr kNumDecoderThreads = 1;
#endif

// Timeout for the router in seconds, used by RoutingTraffDecoder
// TODO set to a sensible value
auto constexpr kRouterTimeoutSec = 30;

/*
 * One meter per second. The TraffEstimator works on distance in meters, not travel time. For code
 * which works with speeds and assumes cost to be time-based, a speed of 1 m/s means such
 * calculations will effectively return distances in meters.
 */
auto constexpr kOneMpSInKmpH = 3.6;

/*
 * Penalty factor for using a fake segment to get to a nearby road.
 * Offroad penalty applies to direct distance whereas road penalty applies to roads, which can be up
 * to around 3 times the direct distance (theoretically unlimited). Therefore, a factor of 3–4 times
 * the penalty of a well-matched road may be needed to avoid competing with the correct route.
 * On the other hand, a very high offroad penalty would give preference to a poorly matched route
 * over a well-matched one if it is closer to the reference points.
 * Maximum penalty for roads is currently 64 (4 for ramps * 4 for road type * 4 for ref).
 * A well-matched road may still have a penalty of around 4 (twice the reduced attribute penalty, or
 * once the full attribute penalty).
 * A “wrong” road may also just have a penalty of 4 (e.g. road name mismatch, but road class and
 * ramp type match).
 * A value of 16 has worked well for the DE-B2R-SendlingSued-Passauerstrasse test case. (The
 * DE-A10-Werder-GrossKreutz or DE-A115-PotsdamDrewitz-Nuthetal test cases gave incorrect results
 * due to lack of fake segments, which was fixed through truncation and now works correctly even
 * with an offroad penalty of 128.)
 */
auto constexpr kOffroadPenalty = 16;

/*
 * Penalty factor for non-matching attributes
 */
auto constexpr kAttributePenalty = 4;

/*
 * Penalty factor for partially matching attributes
 */
auto constexpr kReducedAttributePenalty = 2;

/*
 * Lower and upper boundary for radius around endpoint in which to search for junctions, depending
 * on the road class. Boundaries are in meters. If the lower boundary exceeds half the distance
 * between endpoints, the latter is used instead.
 */
const std::unordered_map<std::optional<RoadClass>, std::array<double, 2>> kJunctionRadiusBoundaries{
  { RoadClass::Motorway, { {300.0, 500.0} } },
  { RoadClass::Trunk, { {300.0, 500.0} } },
  { RoadClass::Primary, { {200.0, 300.0} } },
  { RoadClass::Secondary, { {200.0, 300.0} } },
  { RoadClass::Tertiary, { {200.0, 300.0} } },
  { RoadClass::Other, { {200.0, 300.0} } },
  { std::nullopt, { {300.0, 500.0} } }
};

/*
 * Maximum distance in meters from location endpoint at which a turn penalty is applied
 */
auto constexpr kTurnPenaltyMaxDist = 100.0;

/*
 * Minimum angle in degrees at which turn penalty is applied
 */
auto constexpr kTurnPenaltyMinAngle = 65.0;

/*
 * Minimum angle in degrees at which the full turn penalty is applied
 */
auto constexpr kTurnPenaltyFullAngle = 90.0;

/*
 * Invalid feature ID.
 * Borrowed from indexer/feature_decl.hpp.
 */
uint32_t constexpr kInvalidFeatureId = std::numeric_limits<uint32_t>::max();

TraffDecoder::TraffDecoder(DataSource & dataSource, CountryInfoGetterFn countryInfoGetter,
                           const CountryParentNameGetterFn & countryParentNameGetter,
                           std::map<std::string, TraffMessage> & messageCache)
  : m_dataSource(dataSource)
  , m_countryInfoGetterFn(countryInfoGetter)
  , m_countryParentNameGetterFn(countryParentNameGetter)
  , m_messageCache(messageCache)
{}

void TraffDecoder::ApplyTrafficImpact(traffxml::TrafficImpact & impact, traffxml::MultiMwmColoring & decoded)
{
  traffic::SpeedGroup fromDelay = traffic::SpeedGroup::Unknown;

  /*
   * Convert delay into speed group (skip if the location is blocked).
   * TODO: mapping delay to speed group is not optimal, as a long delay on a short location can
   * increase travel time by several orders of magnitude.
   * Assume a stretch of road with a speed limit of 60 km/h, i.e. 1 km/min, and a delay of 30 min.
   * The lowest speed group, G0, translates to 8% of the posted limit. Since travel time ratio is
   * the inverse of speed ratio, in order to represent 30 seconds of delay, the location would have
   * to be long enough to take ~8% of that time to traverse under normal conditions, i.e. 2.4 min.,
   * or 2.4 km at 60 km/h. For shorter locations, the speed group will underestimate the delay.
   * Higher speeds and longer delays would increase the required length. This is somewhat offset by
   * the fact that the router adds a somewhat arbitrary extra penalty to segments for which traffic
   * problems are reported.
   */
  if ((impact.m_delayMins > 0) && (impact.m_speedGroup != traffic::SpeedGroup::TempBlock))
  {
    double normalDurationS = 0;
    for (auto dit = decoded.begin(); dit != decoded.end(); dit++)
    {
      /*
       * We are initializing two structures for map access here: an MWM handle to get maxspeeds,
       * and a FeaturesLoaderGuard to retrieve points. This may not be very elegant, but works.
       * Since both involve somewhat complex structures, a rewrite might not be simple, although
       * improvements are certainly welcome.
       */
      auto const handle = m_dataSource.GetMwmHandleById(dit->first);
      auto const speeds = routing::LoadMaxspeeds(handle);

      FeaturesLoaderGuard g(m_dataSource, dit->first);
      uint32_t lastFid = kInvalidFeatureId;
      std::vector<m2::PointD> points;
      routing::MaxspeedType speedKmPH = routing::kInvalidSpeed;
      for (auto cit = dit->second.begin(); cit != dit->second.end(); cit++)
      {
        // read points only once per feature ID, not once per segment
        if (lastFid != cit->first.GetFid())
        {
          auto f = g.GetOriginalFeatureByIndex(cit->first.GetFid());
          f->ResetGeometry();
          assign_range(points, f->GetPoints(FeatureType::BEST_GEOMETRY));
          lastFid = cit->first.GetFid();
          auto const speed = speeds->GetMaxspeed(cit->first.GetFid());
          speedKmPH = speed.GetSpeedKmPH(cit->first.GetDir() == traffic::TrafficInfo::RoadSegmentId::kForwardDirection);
        }
        auto const idx = cit->first.GetIdx();
        /*
         * TODO sum of all lengths may differ from route length by up to ~6% in either direction.
         * This is partly due to the fact that route length also includes fake endings, which we are
         * not counting here. However, this only explains routes being longer than the sum of all
         * lengths calculated here, yet in some cases the route is shorter.
         */
        auto const length = mercator::DistanceOnEarth(points[idx], points[idx + 1]);
        if (speedKmPH != routing::kInvalidSpeed)
          normalDurationS += length * kOneMpSInKmpH / speedKmPH;
      }
    }
    auto const delayedDurationS = normalDurationS + impact.m_delayMins * 60;
    fromDelay = traffic::GetSpeedGroupByPercentage(normalDurationS * 100.0f / delayedDurationS);

    LOG(LINFO, ("Normal duration:", normalDurationS, "delayed duration:", delayedDurationS, "speed group:", DebugPrint(fromDelay)));
  }

  for (auto dit = decoded.begin(); dit != decoded.end(); dit++)
  {
    std::unique_ptr<routing::Maxspeeds> speeds = nullptr;
    if ((impact.m_speedGroup != traffic::SpeedGroup::TempBlock) && (impact.m_maxspeed != traffxml::kMaxspeedNone))
    {
      // load maxspeeds once per MWM and only if needed
      auto const handle = m_dataSource.GetMwmHandleById(dit->first);
      speeds = routing::LoadMaxspeeds(handle);
    }
    for (auto cit = dit->second.begin(); cit != dit->second.end(); cit++)
    {
      /*
       * Consolidate TrafficImpact into a single SpeedGroup per segment.
       * Exception: if TrafficImpact already has SpeedGrup::TempBlock, no need to evaluate
       * the rest.
       */
      traffic::SpeedGroup sg = impact.m_speedGroup;

      if ((sg != traffic::SpeedGroup::TempBlock) && (fromDelay != traffic::SpeedGroup::Unknown))
        // process delay
        if ((sg == traffic::SpeedGroup::Unknown) || (fromDelay < sg))
          sg = fromDelay;

      if ((sg != traffic::SpeedGroup::TempBlock) && (impact.m_maxspeed != traffxml::kMaxspeedNone))
      {
        // process maxspeed
        if (speeds)
        {
          traffic::SpeedGroup fromMaxspeed = traffic::SpeedGroup::Unknown;
          auto const speed = speeds->GetMaxspeed(cit->first.GetFid());
          auto const speedKmPH = speed.GetSpeedKmPH(cit->first.GetDir() == traffic::TrafficInfo::RoadSegmentId::kForwardDirection);
          if (speedKmPH != routing::kInvalidSpeed)
          {
            fromMaxspeed = traffic::GetSpeedGroupByPercentage(impact.m_maxspeed * 100.0f / speedKmPH);
            if ((sg == traffic::SpeedGroup::Unknown) || (fromMaxspeed < sg))
              sg = fromMaxspeed;
          }
        }
      }
      cit->second = sg;
    }
  }
}

void TraffDecoder::DecodeMessage(traffxml::TraffMessage & message)
{
  if (!message.m_location)
    return;
  // Decode events into consolidated traffic impact
  std::optional<traffxml::TrafficImpact> impact = message.GetTrafficImpact();

  LOG(LINFO, ("    Impact: ", impact));

  // Skip further processing if there is no impact
  if (!impact)
    return;

  traffxml::MultiMwmColoring decoded;
  bool isDecoded = false;

  std::vector<std::string> ids = message.m_replaces;
  ids.insert(ids.begin(), message.m_id);

  for (auto & id : ids)
  {
    auto it = m_messageCache.find(id);
    if ((it != m_messageCache.end())
        && !it->second.m_decoded.empty()
        && (it->second.m_location == message.m_location))
    {
      // cache already has a message with reusable location

      LOG(LINFO, ("    Location for message", message.m_id, "can be reused from cache"));

      std::optional<traffxml::TrafficImpact> cachedImpact = it->second.GetTrafficImpact();
      if (cachedImpact.has_value() && cachedImpact.value() == impact.value())
      {
        LOG(LINFO, ("    Impact for message", message.m_id, "unchanged, reusing cached coloring"));

        // same impact, m_decoded can be reused altogether
        message.m_decoded = it->second.m_decoded;
        return;
      }
      else if (!isDecoded)
      {
        /*
         * populate only on first occurrence but continue searching, we might find a matching
         * location with matching impact
         */
        decoded = it->second.m_decoded;
        isDecoded = true;
      }
    }
  }
  if (!isDecoded)
    DecodeLocation(message, decoded);

  if (impact)
  {
    ApplyTrafficImpact(impact.value(), decoded);
    std::swap(message.m_decoded, decoded);
  }
}

// Disabled for now, as the OpenLR-based decoder is slow, buggy and not well suited to the task.
#if 0
OpenLrV3TraffDecoder::OpenLrV3TraffDecoder(DataSource & dataSource, CountryInfoGetterFn countryInfoGetter,
                                           const CountryParentNameGetterFn & countryParentNameGetter,
                                           std::map<std::string, TraffMessage> & messageCache)
  : TraffDecoder(dataSource, countryInfoGetter, countryParentNameGetter, messageCache)
  , m_openLrDecoder(dataSource, countryParentNameGetter)
{}

openlr::FunctionalRoadClass OpenLrV3TraffDecoder::GetRoadClassFrc(std::optional<RoadClass> & roadClass)
{
  if (!roadClass)
    return openlr::FunctionalRoadClass::NotAValue;
  switch (roadClass.value())
  {
    case RoadClass::Motorway: return openlr::FunctionalRoadClass::FRC0;
    case RoadClass::Trunk: return openlr::FunctionalRoadClass::FRC0;
    case RoadClass::Primary: return openlr::FunctionalRoadClass::FRC1;
    case RoadClass::Secondary: return openlr::FunctionalRoadClass::FRC2;
    case RoadClass::Tertiary: return openlr::FunctionalRoadClass::FRC3;
    /*
     * TODO Revisit FRC for Other.
     * Other corresponds to FRC4–7.
     * FRC4 matches secondary/tertiary (zero score) and anything below (full score).
     * FRC5–7 match anything below tertiary (full score); secondary/tertiary never match.
     * Primary and above never matches any of these FRCs.
     */
    case RoadClass::Other: return openlr::FunctionalRoadClass::FRC4;
  }
  UNREACHABLE();
}

// TODO tweak formula based on FRC, FOW and direct distance (lower FRC roads may have more and sharper turns)
uint32_t OpenLrV3TraffDecoder::GuessDnp(Point & p1, Point & p2)
{
  // direct distance
  double doe = mercator::DistanceOnEarth(mercator::FromLatLon(p1.m_coordinates),
                                         mercator::FromLatLon(p2.m_coordinates));

  /*
   * Acceptance boundaries for candidate paths are currently:
   *
   * for `openlr::LinearSegmentSource::FromLocationReferenceTag`, 0.6 to ~1.67 (i.e. 1/0.6) times
   * the direct distance,
   *
   * for `openlr::LinearSegmentSource::FromCoordinatesTag`, 0.25 to 4 times the direct distance.
   *
   * A tolerance factor of 1/0.6 is the maximum for which direct distance would be accepted in all
   * cases, with an upper boundary of at least ~2.78 times the direct distance. However, this may
   * cause the actual distance to be overestimated and an incorrect route chosen as a result, as
   * path candidates are scored based on the match between DNP and their length.
   * Also, since we use `openlr::LinearSegmentSource::FromCoordinatesTag`, acceptance limits are
   * much wider than that.
   * In practice, the shortest route from one valley to the next in a mountain area is seldom more
   * than 3 times the direct distance, based on a brief examination. This would be even within the
   * limits of direct distance, hence we do not need a large correction factor for this scenario.
   *
   * Candidate values:
   * 1.66 (1/0.6) – upper boundary for direct distance to be just within the most stringent limits
   * 1.41 (2^0.5) – ratio between two sides of a square and its diagonal
   * 1.3 – close to the square root of 1.66 (halfway between 1 and 1.66)
   * 1.19 – close to the square root of 1.41
   * 1 – direct distance unmodified
   */
  double dist = doe * 1.19f;

  // if we have kilometric points, calculate nominal distance as the difference between them
  if (p1.m_distance && p2.m_distance)
  {
    LOG(LINFO, ("Both points have distance, calculating nominal difference"));
    float nominalDist = (p1.m_distance.value() - p2.m_distance.value()) * 1000.0;
    if (nominalDist < 0)
      nominalDist *= -1;
    /*
     * Plausibility check for nominal distance, as kilometric points along the route may not be
     * continuous: discard if shorter than direct distance (geometrically impossible) or if longer
     * than 4 times direct distance (somewhat arbitrary, based on the OpenLR acceptance limit for
     * `openlr::LinearSegmentSource::FromCoordinatesTag`, as well as real-world observations of
     * distances between two adjacent mountain valleys, which are up to roughly 3 times the direct
     * distance). If nominal distance is outside these boundaries, discard it and use `dist` (direct
     * distance with a tolerance factor).
     */
    if ((nominalDist >= doe) && (nominalDist <= doe * 4))
      dist = nominalDist;
    else
      LOG(LINFO, ("Nominal distance:", nominalDist, "direct distance:", doe, "– discarding"));
  }
  return dist + 0.5f;
}

openlr::LocationReferencePoint OpenLrV3TraffDecoder::PointToLrp(Point & point)
{
  openlr::LocationReferencePoint result;
  result.m_latLon = ms::LatLon(point.m_coordinates.m_lat, point.m_coordinates.m_lon);
  return result;
}

openlr::LinearLocationReference OpenLrV3TraffDecoder::TraffLocationToLinearLocationReference(TraffLocation & location, bool backwards)
{
  openlr::LinearLocationReference locationReference;
  locationReference.m_points.clear();
  std::vector<Point> points;
  if (location.m_from)
    points.push_back(location.m_from.value());
  if (location.m_at)
    points.push_back(location.m_at.value());
  else if (location.m_via)
    points.push_back(location.m_via.value());
  if (location.m_to)
    points.push_back(location.m_to.value());
  if (backwards)
    std::reverse(points.begin(), points.end());
  // m_notVia is ignored as OpenLR does not support this functionality.
  CHECK_GREATER(points.size(), 1, ("At least two reference points must be given"));
  for (size_t i = 0; i < points.size(); i++)
  {
    openlr::LocationReferencePoint lrp = PointToLrp(points[i]);
    lrp.m_functionalRoadClass = GetRoadClassFrc(location.m_roadClass);
    if (location.m_ramps != traffxml::Ramps::None)
      lrp.m_formOfWay = openlr::FormOfWay::Sliproad;
    if (i < points.size() - 1)
    {
      lrp.m_distanceToNextPoint
          = GuessDnp(points[i], points[i + 1]);
      /*
       * Somewhat hackish. LFRCNP is evaluated by the same function as FRC and the candidate is
       * used or discarded based on whether a score was returned or not (the score itself is not
       * used for LFRCNP). However, this means we can use FRC as LFRCNP.
       */
      lrp.m_lfrcnp = GetRoadClassFrc(location.m_roadClass);
    }
    locationReference.m_points.push_back(lrp);
  }
  return locationReference;
}

// TODO make segment ID in OpenLR a string value, and store messageId
std::vector<openlr::LinearSegment> OpenLrV3TraffDecoder::TraffLocationToOpenLrSegments(TraffLocation & location, std::string & messageId)
{
  // Convert the location to a format understood by the OpenLR decoder.
  std::vector<openlr::LinearSegment> segments;
  int dirs = (location.m_directionality == Directionality::BothDirections) ? 2 : 1;
  for (int dir = 0; dir < dirs; dir++)
  {
    openlr::LinearSegment segment;
    /*
     * Segment IDs are used internally by the decoder but nowhere else.
     * Since we decode TraFF locations one at a time, there are at most two segments in a single
     * decoder instance (one segment per direction). Therefore, a segment ID derived from the
     * direction is unique within the decoder instance.
     */
    segment.m_segmentId = dir;
    segment.m_messageId = messageId;
    /*
     * Segments generated from coordinates can have any number of points. Each point, except for
     * the last point, must indicate the distance to the next point. Line properties (functional
     * road class (FRC), form of way, bearing) or path properties other than distance to next point
     * (lowest FRC to next point, againstDrivingDirection) are ignored.
     * Segment length is never evaluated.
     * TODO update OpenLR decoder to make all line and path properties optional.
     */
    segment.m_source = openlr::LinearSegmentSource::FromCoordinatesTag;
    segment.m_locationReference = TraffLocationToLinearLocationReference(location, dir == 0 ? false : true);

    segments.push_back(segment);
  }
  return segments;
}

/*
 * TODO the OpenLR decoder is designed to handle multiple segments (i.e. locations).
 * Decoding message by message kind of defeats the purpose.
 * But after decoding the location, we need to examine the map features we got in order to
 * determine the speed groups, thus we may need to decode one by one (TBD).
 * If we batch-decode segments, we need to fix the [partner] segment IDs in the segment and path
 * structures to accept a TraFF message ID (string) rather than an integer, or derive
 * [partner] segment IDs from TraFF message IDs.
 */
void OpenLrV3TraffDecoder::DecodeLocation(traffxml::TraffMessage & message, traffxml::MultiMwmColoring & decoded)
{
  ASSERT(message.m_location, ("Message has no location"));
  decoded.clear();

  // Convert the location to a format understood by the OpenLR decoder.
  std::vector<openlr::LinearSegment> segments
      = TraffLocationToOpenLrSegments(message.m_location.value(), message.m_id);

  for (auto segment : segments)
  {
    LOG(LINFO, ("    Segment:", segment.m_segmentId));
    for (size_t i = 0; i < segment.m_locationReference.m_points.size(); i++)
    {
      LOG(LINFO, ("     ", i, ":", segment.m_locationReference.m_points[i].m_latLon));
      if (i < segment.m_locationReference.m_points.size() - 1)
      {
        LOG(LINFO, ("        FRC:", segment.m_locationReference.m_points[i].m_functionalRoadClass));
        LOG(LINFO, ("        DNP:", segment.m_locationReference.m_points[i].m_distanceToNextPoint));
      }
    }
  }

  // Decode the location into a path on the map.
  // One path per segment
  std::vector<openlr::DecodedPath> paths(segments.size());
  m_openLrDecoder.DecodeV3(segments, kNumDecoderThreads, paths);

  for (size_t i = 0; i < paths.size(); i++)
    for (size_t j = 0; j < paths[i].m_path.size(); j++)
    {
      auto fid = paths[i].m_path[j].GetFeatureId().m_index;
      auto segment = paths[i].m_path[j].GetSegId();
      uint8_t direction = paths[i].m_path[j].IsForward() ?
                          traffic::TrafficInfo::RoadSegmentId::kForwardDirection :
                          traffic::TrafficInfo::RoadSegmentId::kReverseDirection;
      decoded[paths[i].m_path[j].GetFeatureId().m_mwmId][traffic::TrafficInfo::RoadSegmentId(fid, segment, direction)] = traffic::SpeedGroup::Unknown;
    }
}
#endif

double RoutingTraffDecoder::TraffEstimator::GetUTurnPenalty(Purpose /* purpose */) const
{
  // Adds 2 minutes penalty for U-turn. The value is quite arbitrary
  // and needs to be properly selected after a number of real-world
  // experiments.
  return 2 * 60;  // seconds
}

double RoutingTraffDecoder::TraffEstimator::GetTurnPenalty(Purpose /* purpose */, double angle,
                                                             routing::RoadGeometry const & from_road,
                                                             routing::RoadGeometry const & to_road,
                                                             bool is_left_hand_traffic) const
{
  // Flip sign for left-hand traffic, so a positive angle always means a turn across traffic
  if (is_left_hand_traffic)
    angle *= -1;

  // We only penalize sharp turns (above kTurnPenaltyMinAngle) across traffic
  if (angle < kTurnPenaltyMinAngle)
    return 0.0;

  /*
   * Identify coordinates of location endpoints and of the turn, and establish distance between the
   * turn and the nearest endpoint.
   */
  ms::LatLon from = m_decoder.m_message.value().m_location.value().m_from
      ? m_decoder.m_message.value().m_location.value().m_from.value().m_coordinates
          : m_decoder.m_message.value().m_location.value().m_at.value().m_coordinates;
  ms::LatLon to = m_decoder.m_message.value().m_location.value().m_to
      ? m_decoder.m_message.value().m_location.value().m_to.value().m_coordinates
          : m_decoder.m_message.value().m_location.value().m_at.value().m_coordinates;

  // Upper boundary for distance (approximately earth circumference)
  double dist = 4.0e+7;

  for (auto & fromPoint : { from_road.GetPoint(0), from_road.GetPoint(from_road.GetPointsCount() - 1) })
    for (auto & toPoint : { to_road.GetPoint(0), to_road.GetPoint(to_road.GetPointsCount() - 1) })
      if (fromPoint == toPoint)
        for (auto & endpoint : { from, to })
        {
          auto newdist = ms::DistanceOnEarth(fromPoint, endpoint);
          if (newdist < dist)
            dist = newdist;
        }

  // We only penalize turns close to an endpoint
  if (dist > kTurnPenaltyMaxDist)
    return 0.0;

  /*
   * The penalty depends on the distance between the turn point and the nearest endpoint: the
   * shorter the distance, the greater the penalty. This is obtained by subtracting the distance
   * from `kTurnPenaltyMaxDist`.
   *
   * Above `kTurnPenaltyFullAngle`, the full turn penalty applies, i.e. the distance-based value is
   * multiplied with `kAttributePenalty`.
   *
   * Between `kTurnPenaltyMinAngle` and `kTurnPenaltyFullAngle`, the penalty proportionally
   * increases from 0 to the full value.
   */
  double result = (kTurnPenaltyMaxDist - dist) * kAttributePenalty;
  if (angle < kTurnPenaltyFullAngle)
    result *= (angle - kTurnPenaltyMinAngle) / (kTurnPenaltyFullAngle - kTurnPenaltyMinAngle);
  return result;
}

double RoutingTraffDecoder::TraffEstimator::GetFerryLandingPenalty(Purpose /* purpose */) const
{
  return 20 * 60;   // seconds
}

double RoutingTraffDecoder::TraffEstimator::CalcOffroad(ms::LatLon const & from, ms::LatLon const & to,
                                  Purpose /* purpose */) const
{
  /*
   * Usage of this method is not quite clear. For some locations this method never gets called.
   * There is also no clear pattern in which of the two arguments is the reference point and which
   * is part of a segment. Either reference point can appear as either argument for either direction,
   * nothing to infer from a particular reference point appearing in a particular argument.
   */

  double defaultWeight = ms::DistanceOnEarth(from, to) * kOffroadPenalty;

  /*
   * Retrieves offroad weight from the junctions map supplied, if found, or default.
   *
   * Bugs: Due to back-and-forth conversion of `roadPoint` from Mercator to WGS84 and back, it may
   * no longer match its counterpart in `junctions` (near-miss).
   *
   * Tests showed very few actual matches. Extending this logic to return near-matches did return
   * some more, but still relatively few. This may be due to the way fake segments are chosen.
   *
   * refPoint: point from TraFF location
   * roadPoint: point on segment
   * junctions: known junctions for `refPoint`
   *
   * Returns: reduced offroad weight from table, or default offroad weight if not found
   */
  auto const getOffroadFromJunction = [defaultWeight](ms::LatLon const & refPoint,
      ms::LatLon const & roadPoint,
      std::map<m2::PointD, double> const & junctions)
  {
    m2::PointD m2RoadPoint = mercator::FromLatLon(roadPoint);
    auto it = junctions.find(m2RoadPoint);
    if (it != junctions.end())
      return it->second;
    // TODO this is likely an inefficient way to return near-matches
    for (auto & [point, weight] : junctions)
      if (m2RoadPoint.EqualDxDy(point, kMwmPointAccuracy))
        return weight;
    return defaultWeight;
  };

  /*
   * If one of from/to is a reference point and the other is in the corresponding junction map,
   * return the weight from the map
   */
  if (m_decoder.m_message.value().m_location.value().m_from)
  {
    if (m_decoder.m_message.value().m_location.value().m_from.value().m_coordinates == from)
      return getOffroadFromJunction(from, to, m_decoder.m_startJunctions);
    else if (m_decoder.m_message.value().m_location.value().m_from.value().m_coordinates == to)
      return getOffroadFromJunction(to, from, m_decoder.m_startJunctions);
  }
  if (m_decoder.m_message.value().m_location.value().m_to)
  {
    if (m_decoder.m_message.value().m_location.value().m_to.value().m_coordinates == from)
      return getOffroadFromJunction(from, to, m_decoder.m_endJunctions);
    else if (m_decoder.m_message.value().m_location.value().m_to.value().m_coordinates == to)
      return getOffroadFromJunction(to, from, m_decoder.m_endJunctions);
  }

  return defaultWeight;
}

/*
 * Currently, the attribute penalty (kAttributePenalty or kReducedAttributePenalty) can be applied
 * up to 3 times:
 * - ramp attribute mismatch
 * - road class mismatch
 * - road ref mismatch
 */
double RoutingTraffDecoder::TraffEstimator::CalcSegmentWeight(routing::Segment const & segment, routing::RoadGeometry const & road, Purpose /* purpose */) const
{
  double result = road.GetDistance(segment.GetSegmentIdx());

  if (!m_decoder.m_message || !m_decoder.m_message.value().m_location.value().m_roadClass)
    return result;

  result *= GetHighwayTypePenalty(road.GetHighwayType(),
                                  m_decoder.m_message.value().m_location.value().m_roadClass,
                                  m_decoder.m_message.value().m_location.value().m_ramps);

  if (!m_decoder.m_roadRef.empty())
  {
    auto const countryFile = m_decoder.m_numMwmIds->GetFile(segment.GetMwmId());
    auto const mwmId = m_decoder.m_dataSource.GetMwmIdByCountryFile(countryFile);
    FeaturesLoaderGuard g(m_decoder.m_dataSource, mwmId);
    auto f = g.GetOriginalFeatureByIndex(segment.GetFeatureId());
    auto refs = ftypes::GetRoadShieldsNames(*f);

    result *= m_decoder.GetRoadRefPenalty(refs);
  }

  return result;
}

RoutingTraffDecoder::DecoderRouter::DecoderRouter(CountryParentNameGetterFn const & countryParentNameGetterFn,
                                                  routing::TCountryFileFn const & countryFileFn,
                                                  routing::CountryRectFn const & countryRectFn,
                                                  std::shared_ptr<routing::NumMwmIds> numMwmIds,
                                                  std::unique_ptr<m4::Tree<routing::NumMwmId>> numMwmTree,
                                                  DataSource & dataSource, RoutingTraffDecoder & decoder)
  : routing::IndexRouter(routing::VehicleType::Car /* VehicleType vehicleType */,
                         false /* bool loadAltitudes */,
                         countryParentNameGetterFn,
                         countryFileFn,
                         countryRectFn,
                         numMwmIds,
                         std::move(numMwmTree),
                         //std::nullopt /* std::optional<traffic::TrafficCache> const & trafficCache */,
                         std::make_shared<TraffEstimator>(&dataSource, numMwmIds, kOneMpSInKmpH /* maxWeighSpeedKMpH */,
                                                          routing::SpeedKMpH(kOneMpSInKmpH / kOffroadPenalty /* weight */,
                                                                             routing::kNotUsed /* eta */) /* offroadSpeedKMpH */,
                                                          decoder),
                         dataSource)
  //, m_directionsEngine(CreateDirectionsEngine(m_vehicleType, m_numMwmIds, m_dataSource)) // TODO we don’t need directions, can we disable that?
{}

routing::RoutingOptions RoutingTraffDecoder::DecoderRouter::GetRoutingOptions()
{
  return routing::RoutingOptions();
}

RoutingTraffDecoder::RoutingTraffDecoder(DataSource & dataSource, CountryInfoGetterFn countryInfoGetter,
                                         const CountryParentNameGetterFn & countryParentNameGetter,
                                         std::map<std::string, TraffMessage> & messageCache)
  : TraffDecoder(dataSource, countryInfoGetter, countryParentNameGetter, messageCache)
{
  m_dataSource.AddObserver(*this);
  InitRouter();
}

double RoutingTraffDecoder::GetHighwayTypePenalty(std::optional<routing::HighwayType> highwayType,
                                                  std::optional<RoadClass> roadClass,
                                                  Ramps ramps)
{
  double result = 1.0;
  if (highwayType)
  {
    if (IsRamp(highwayType.value()) != (ramps != Ramps::None))
      // if one is a ramp and the other is not, treat it as a mismatch
      result *= kAttributePenalty;
    if (roadClass)
      // if the message specifies a road class, penalize mismatches
      result *= GetRoadClassPenalty(roadClass.value(), GetRoadClass(highwayType.value()));
  }
  else // road has no highway class
  {
    // we can’t determine if it is a ramp, penalize for mismatch
    result *= kAttributePenalty;
    if (roadClass)
      // we can’t determine if the road matches the required road class, treat it as mismatch
      result *= kAttributePenalty;
  }
  return result;
}

double RoutingTraffDecoder::GetRoadRefPenalty(std::vector<std::string> & refs) const
{
  double result = kAttributePenalty;

  for (auto & ref : refs)
  {
    auto newResult = GetRoadRefPenalty(ref);
    if (newResult < result)
      result = newResult;
    if (result == 1)
      break;
  }

  return result;
}

double RoutingTraffDecoder::GetRoadRefPenalty(std::string const & ref) const
{
  // skip parsing if ref is empty
  if (ref.empty())
  {
    if (m_roadRef.empty())
      return 1;
    else if (!m_roadRef.empty())
      return kAttributePenalty;
  }

  // TODO does caching results per ref improve performance?

  std::vector<std::string> r = ParseRef(ref);

  size_t matches = 0;

  if (m_roadRef.empty() && r.empty())
    return 1;
  else if (m_roadRef.empty() || r.empty())
    return kAttributePenalty;

  // work on a copy of `m_decoder.m_roadRef`
  std::vector<std::string> l = m_roadRef;

  if ((l.size() > 1) && (r.size() > 1) && (l.front() == r.front()))
  {
    /*
     * Discard generic prefixes, which are often used to denote the road class.
     * This will turn `A1` and `A2` into `1` and `2`, causing them to be treated as a mismatch,
     * not a partial match.
     */
    l.erase(l.begin());
    r.erase(r.begin());
  }

  // for both sides, count items matched by the other side
  for (auto & litem : l)
    for (auto ritem : r)
      if (litem == ritem)
      {
        matches++;
        break;
      }

  for (auto ritem : r)
    for (auto & litem : l)
      if (litem == ritem)
      {
        matches++;
        break;
      }

  if (matches == 0)
    return kAttributePenalty;
  else if (matches == (l.size() + r.size()))
    return 1;
  else
    return kReducedAttributePenalty;
}

void RoutingTraffDecoder::OnMapRegistered(platform::LocalCountryFile const & localFile)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  // register with our router instance, unless it is World or WorldCoasts.
  if (!localFile.GetCountryName().starts_with(WORLD_FILE_NAME))
    m_numMwmIds->RegisterFile(localFile.GetCountryFile());
}

bool RoutingTraffDecoder::InitRouter()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_router)
    return true;

  /*
   * Code based on RoutingManager::SetRouterImpl(RouterType), which calls
   * m_delegate.RegisterCountryFilesOnRoute(numMwmIds); m_delegate being the framework instance.
   * RegisterCountryFilesOnRoute() is protected and uses a private `Storage` instance.
   * We therefore have to resort to populating `m_numMwmIds` from `m_dataSource`. Unlike the
   * “original”, this will only include MWMs loaded on startup, not those added later.
   * For these, we register ourselves as an MwmSet::Observer and add maps to `m_numMwms` as they
   * are registered.
   * World and WorldCoasts must be excluded (as in the original routine), as they would cause the
   * router to return bogus routes. Just like the original, we use a string comparison for this.
   */
  std::vector<std::shared_ptr<MwmInfo>> mwmsInfo;
  m_dataSource.GetMwmsInfo(mwmsInfo);

  for (auto mwmInfo : mwmsInfo)
    if (!mwmInfo->GetCountryName().starts_with(WORLD_FILE_NAME))
      m_numMwmIds->RegisterFile(mwmInfo->GetLocalFile().GetCountryFile());

  if (m_numMwmIds->IsEmpty())
    return false;

  auto const countryFileGetter = [this](m2::PointD const & p) -> std::string {
    // TODO (@gorshenin): fix CountryInfoGetter to return CountryFile
    // instances instead of plain strings.
    return m_countryInfoGetterFn().GetRegionCountryId(p);
  };

  auto const getMwmRectByName = [this](std::string const & countryId) -> m2::RectD {
    return m_countryInfoGetterFn().GetLimitRectForLeaf(countryId);
  };

  m_router =
      make_unique<RoutingTraffDecoder::DecoderRouter>(m_countryParentNameGetterFn,
                                                      countryFileGetter, getMwmRectByName, m_numMwmIds,
                                                      routing::MakeNumMwmTree(*m_numMwmIds, m_countryInfoGetterFn()),
                                                      m_dataSource, *this);

  return true;
}

// Copied from AsyncRouter
// static
void RoutingTraffDecoder::LogCode(routing::RouterResultCode code, double const elapsedSec)
{
  switch (code)
  {
    case routing::RouterResultCode::StartPointNotFound:
      LOG(LWARNING, ("Can't find start or end node"));
      break;
    case routing::RouterResultCode::EndPointNotFound:
      LOG(LWARNING, ("Can't find end point node"));
      break;
    case routing::RouterResultCode::PointsInDifferentMWM:
      LOG(LWARNING, ("Points are in different MWMs"));
      break;
    case routing::RouterResultCode::RouteNotFound:
      LOG(LWARNING, ("Route not found"));
      break;
    case routing::RouterResultCode::RouteFileNotExist:
      LOG(LWARNING, ("There is no routing file"));
      break;
    case routing::RouterResultCode::NeedMoreMaps:
      LOG(LINFO,
          ("Routing can find a better way with additional maps, elapsed seconds:", elapsedSec));
      break;
    case routing::RouterResultCode::Cancelled:
      LOG(LINFO, ("Route calculation cancelled, elapsed seconds:", elapsedSec));
      break;
    case routing::RouterResultCode::NoError:
      LOG(LINFO, ("Route found, elapsed seconds:", elapsedSec));
      break;
    case routing::RouterResultCode::NoCurrentPosition:
      LOG(LINFO, ("No current position"));
      break;
    case routing::RouterResultCode::InconsistentMWMandRoute:
      LOG(LINFO, ("Inconsistent mwm and route"));
      break;
    case routing::RouterResultCode::InternalError:
      LOG(LINFO, ("Internal error"));
      break;
    case routing::RouterResultCode::FileTooOld:
      LOG(LINFO, ("File too old"));
      break;
    case routing::RouterResultCode::IntermediatePointNotFound:
      LOG(LWARNING, ("Can't find intermediate point node"));
      break;
    case routing::RouterResultCode::TransitRouteNotFoundNoNetwork:
      LOG(LWARNING, ("No transit route is found because there's no transit network in the mwm of "
                     "the route point"));
      break;
    case routing::RouterResultCode::TransitRouteNotFoundTooLongPedestrian:
      LOG(LWARNING, ("No transit route is found because pedestrian way is too long"));
      break;
    case routing::RouterResultCode::RouteNotFoundRedressRouteError:
      LOG(LWARNING, ("Route not found because of a redress route error"));
      break;
  case routing::RouterResultCode::HasWarnings:
      LOG(LINFO, ("Route has warnings, elapsed seconds:", elapsedSec));
      break;
  }
}

void RoutingTraffDecoder::AddDecodedSegment(traffxml::MultiMwmColoring & decoded, routing::Segment & segment)
{
  auto const countryFile = m_numMwmIds->GetFile(segment.GetMwmId());
  MwmSet::MwmId mwmId = m_dataSource.GetMwmIdByCountryFile(countryFile);

  auto const fid = segment.GetFeatureId();
  auto const sid = segment.GetSegmentIdx();
  uint8_t direction = segment.IsForward() ?
        traffic::TrafficInfo::RoadSegmentId::kForwardDirection :
        traffic::TrafficInfo::RoadSegmentId::kReverseDirection;

  decoded[mwmId][traffic::TrafficInfo::RoadSegmentId(fid, sid, direction)] = traffic::SpeedGroup::Unknown;
}

void RoutingTraffDecoder::TruncateRoute(std::vector<routing::RouteSegment> & rsegments,
                                        routing::Checkpoints const & checkpoints, bool backwards)
{
  double startWeight = 0;
  double const endWeight = rsegments.back().GetTimeFromBeginningSec();

  // erase leading and trailing fake segments
  while(!rsegments.empty() && rsegments.front().GetSegment().GetMwmId() == routing::kFakeNumMwmId)
  {
    startWeight = rsegments.front().GetTimeFromBeginningSec();
    rsegments.erase(rsegments.begin());
  }
  while(!rsegments.empty() && rsegments.back().GetSegment().GetMwmId() == routing::kFakeNumMwmId)
    rsegments.pop_back();

  if (rsegments.size() < 2)
    return;

  // Index of first segment to keep, or number of segments to truncate at start.
  size_t start = 0;
  // Cost saved by omitting all segments prior to `start`.
  double startSaving = 0;

  // Index of last segment to keep.
  size_t end = rsegments.size() - 1;
  // Cost saved by omitting the last `end` segments.
  double endSaving = 0;

  TruncateStart(rsegments, checkpoints, start, startSaving, startWeight,
                backwards ? m_endJunctions : m_startJunctions);
  TruncateEnd(rsegments, checkpoints, end, endSaving, endWeight,
              backwards ? m_startJunctions : m_endJunctions);

  /*
   * If start <= end, we can truncate both ends at the same time.
   * Else, the segments to truncate overlap. In this case, first truncate where the saving is bigger,
   * then recalculate the other end and truncate it as well.
   */
  if (start <= end)
  {
    rsegments.erase(rsegments.begin() + end + 1, rsegments.end());
    rsegments.erase(rsegments.begin(), rsegments.begin() + start);
  }
  else if (startSaving > endSaving)
  {
    // truncate start, then recalculate and truncate end
    rsegments.erase(rsegments.begin(), rsegments.begin() + start);
    end = rsegments.size() - 1;
    endSaving = 0;
    TruncateEnd(rsegments, checkpoints, end, endSaving, endWeight,
                backwards ? m_startJunctions : m_endJunctions);
    rsegments.erase(rsegments.begin() + end + 1, rsegments.end());
  }
  else
  {
    // truncate end, then recalculate and truncate start
    rsegments.erase(rsegments.begin() + end + 1, rsegments.end());
    start = 0;
    startSaving = 0;
    TruncateStart(rsegments, checkpoints, start, startSaving, startWeight,
                  backwards ? m_endJunctions : m_startJunctions);
    rsegments.erase(rsegments.begin(), rsegments.begin() + start);
  }
}

void RoutingTraffDecoder::DecodeLocationDirection(traffxml::TraffMessage & message,
                                                  traffxml::MultiMwmColoring & decoded, bool backwards)
{
  bool adjustToPrevRoute = false; // calculate a fresh route, no adjustments to previous one
  uint64_t routeId = 0; // used in callbacks to identify the route, we might not need it at all

  std::vector<m2::PointD> points;
  if (message.m_location.value().m_from)
    points.push_back(mercator::FromLatLon(message.m_location.value().m_from.value().m_coordinates));
  if (message.m_location.value().m_at)
    points.push_back(mercator::FromLatLon(message.m_location.value().m_at.value().m_coordinates));
  else if (message.m_location.value().m_via)
    points.push_back(mercator::FromLatLon(message.m_location.value().m_via.value().m_coordinates));
  if (message.m_location.value().m_to)
    points.push_back(mercator::FromLatLon(message.m_location.value().m_to.value().m_coordinates));
  if (backwards)
    std::reverse(points.begin(), points.end());
  // m_notVia is ignored as OpenLR does not support this functionality.
  CHECK_GREATER(points.size(), 1, ("At least two reference points must be given"));

  /*
   * startDirection is the direction of travel at start. Can be m2::PointD::Zero() to ignore
   * direction, or PositionAccumulator::GetDirection(), which basically returns the difference
   * between the last position and an earlier one (offset between two points from which the
   * direction of travel can be inferred).
   *
   * For our purposes, points[1] - points[0] would be as close as we could get to a start direction.
   * This would be accurate on very straight roads, less accurate on not-so-straight ones. However,
   * even on a near-straight road, the standard router (with the default EdgeEstimator) seemed quite
   * unimpressed by the direction and insisted on starting off on the carriageway closest to the
   * start point and sending us on a huge detour, instead of taking the direct route on the opposite
   * carriageway.
   */
  m2::PointD startDirection = m2::PointD::Zero();

  routing::Checkpoints checkpoints(std::move(points));

  /*
   * This code is mostly lifted from AsyncRouter::CalculateRoute(), both with and without arguments.
   */

  /*
   * AsyncRouter::CalculateRoute() has a `DelegateProxy`, which is private. We just need the return
   * value of GetDelegate(), which is a `routing::RouterDelegate`, so use that instead. We don’t
   * need any of the callbacks, therefore we don’t set them.
   */
  routing::RouterDelegate delegate;
  delegate.SetTimeout(kRouterTimeoutSec);
  routing::RouterResultCode code;
  std::shared_ptr<routing::Route> route;

  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_router && !InitRouter())
      return;

    /*
   * TODO is that for following a track? If so, can we use that with just 2–3 reference points?
   * – Doesn’t look like it, m_guides only seems to get used in test functions
   */
    //router->SetGuides(std::move(m_guides));
    //m_guides.clear();

    route = std::make_shared<routing::Route>(m_router->GetName(), routeId);

    base::Timer timer;
    double elapsedSec = 0.0;

    try
    {
      LOG(LINFO, ("Calculating the route of direct length", checkpoints.GetSummaryLengthBetweenPointsMeters(),
                  "m. checkpoints:", checkpoints, "startDirection:", startDirection, "router name:", m_router->GetName()));

      // Run basic request.
      code = m_router->CalculateRoute(checkpoints, startDirection, adjustToPrevRoute,
                                      delegate, *route);
      m_router->SetGuides({});
      elapsedSec = timer.ElapsedSeconds(); // routing time
      LogCode(code, elapsedSec);
      LOG(LINFO, ("ETA:", route->GetTotalTimeSec(), "sec."));
    }
    catch (RootException const & e)
    {
      code = routing::RouterResultCode::InternalError;
      LOG(LERROR, ("Exception happened while calculating route:", e.Msg()));
      return;
    }
  }

  if (code == routing::RouterResultCode::NoError)
  {
    std::vector<routing::RouteSegment> rsegments(route->GetRouteSegments());

    TruncateRoute(rsegments, checkpoints, backwards);

    /*
     * `m_onRoundabout` is set only for the first segment after the junction. In order to identify
     * all roundabout segments, cache the last segment with `m_onRoundabout` set. Any subsequent
     * segment with the same MWM and feature ID is also a roundabout segment.
     */
    routing::Segment lastRoundaboutSegment;

    /*
     * We usually discard roundabouts, unless the location is a point (`at` point set) or the entire
     * decoded location is a roundabout.
     */
    bool keepRoundabouts = true;

    if (!message.m_location.value().m_at)
      for (auto & rsegment : rsegments)
      {
        if (rsegment.GetRoadNameInfo().m_onRoundabout)
          lastRoundaboutSegment = rsegment.GetSegment();
        else if ((rsegment.GetSegment().GetMwmId() != lastRoundaboutSegment.GetMwmId())
            || (rsegment.GetSegment().GetFeatureId() != lastRoundaboutSegment.GetFeatureId()))
        {
          keepRoundabouts = false;
          break;
        }
      }

    if (!backwards && message.m_location.value().m_at && !message.m_location.value().m_to)
      // from–at in forward direction, add last segment
      AddDecodedSegment(decoded, rsegments.back().GetSegment());
    else if (!backwards && message.m_location.value().m_at && !message.m_location.value().m_from)
      // at–to in forward direction, add last segment
      AddDecodedSegment(decoded, rsegments.front().GetSegment());
    else if (backwards && message.m_location.value().m_at && !message.m_location.value().m_to)
      // from–at in backward direction, add first segment
      AddDecodedSegment(decoded, rsegments.front().GetSegment());
    else if (backwards && message.m_location.value().m_at && !message.m_location.value().m_from)
      // at–to in backward direction, add first segment
      AddDecodedSegment(decoded, rsegments.back().GetSegment());
    else if (message.m_location.value().m_at)
    {
      // from–at–to, find closest segment
      ms::LatLon at = message.m_location.value().m_at.value().m_coordinates;
      routing::RouteSegment & closestRSegment = rsegments.front();
      double closestDist = ms::DistanceOnEarth(at, mercator::ToLatLon(closestRSegment.GetJunction().GetPoint()));

      for (auto & rsegment : rsegments)
      {
        // If we have more than two checkpoints, fake segments can occur in the middle, skip them.
        if (rsegment.GetSegment().GetMwmId() == routing::kFakeNumMwmId)
          continue;

        double dist = ms::DistanceOnEarth(at, mercator::ToLatLon(rsegment.GetJunction().GetPoint()));
        if (dist < closestDist)
        {
          closestRSegment = rsegment;
          closestDist = dist;
        }
      }
      AddDecodedSegment(decoded, closestRSegment.GetSegment());
    }
    else
      // from–[via]–to, add all real segments
      for (auto & rsegment : rsegments)
      {
        routing::Segment & segment = rsegment.GetSegment();

        // Skip roundabouts to avoid side effects on crossing roads
        if (!keepRoundabouts)
        {
          if (rsegment.GetRoadNameInfo().m_onRoundabout)
          {
            lastRoundaboutSegment = segment;
            continue;
          }
          else if ((segment.GetMwmId() == lastRoundaboutSegment.GetMwmId())
              && (segment.GetFeatureId() == lastRoundaboutSegment.GetFeatureId()))
            continue;
        }

        // If we have more than two checkpoints, fake segments can occur in the middle, skip them.
        if (segment.GetMwmId() == routing::kFakeNumMwmId)
          continue;

        AddDecodedSegment(decoded, segment);
      }
  }
}

void RoutingTraffDecoder::DecodeLocation(traffxml::TraffMessage & message, traffxml::MultiMwmColoring & decoded)
{
  ASSERT(message.m_location, ("Message has no location"));
  decoded.clear();

  m_message = message;

  if (m_message.value().m_location.value().m_roadRef)
    m_roadRef = ParseRef(m_message.value().m_location.value().m_roadRef.value());
  else
    m_roadRef.clear();

  GetJunctionPointCandidates();

  int dirs = (message.m_location.value().m_directionality == Directionality::BothDirections) ? 2 : 1;
  for (int dir = 0; dir < dirs; dir++)
    DecodeLocationDirection(message, decoded, dir == 0 ? false : true /* backwards */);

  m_message = std::nullopt;
  m_roadRef.clear();
}

void RoutingTraffDecoder::GetJunctionPointCandidates()
{
  m_startJunctions.clear();
  m_endJunctions.clear();

  if (m_message.value().m_location.value().m_fuzziness
      && (m_message.value().m_location.value().m_fuzziness.value() == traffxml::Fuzziness::LowRes))
  {
    /*
     * Identify coordinates of location endpoints and of the turn, and determine distance.
     */
    ms::LatLon from = m_message.value().m_location.value().m_from
        ? m_message.value().m_location.value().m_from.value().m_coordinates
            : m_message.value().m_location.value().m_at.value().m_coordinates;
    ms::LatLon to = m_message.value().m_location.value().m_to
        ? m_message.value().m_location.value().m_to.value().m_coordinates
            : m_message.value().m_location.value().m_at.value().m_coordinates;

    auto dist = ms::DistanceOnEarth(from, to);

    auto bounds = kJunctionRadiusBoundaries.at(m_message.value().m_location.value().m_roadClass);

    m_junctionRadius = dist / 3.0f;
    if (m_junctionRadius > bounds[1])
      m_junctionRadius = bounds[1];
    else if (m_junctionRadius < bounds[0])
    {
      m_junctionRadius = dist / 2.0f;
      if (m_junctionRadius > bounds[0])
        m_junctionRadius = bounds[0];
    }

    if (m_message.value().m_location.value().m_from)
      GetJunctionPointCandidates(m_message.value().m_location.value().m_from.value(), m_startJunctions);
    if (m_message.value().m_location.value().m_to)
      GetJunctionPointCandidates(m_message.value().m_location.value().m_to.value(), m_endJunctions);
  }
}

void RoutingTraffDecoder::GetJunctionPointCandidates(Point const & point,
                                                     std::map<m2::PointD, double> & junctions)
{
  m2::PointD const m2Point = mercator::FromLatLon(point.m_coordinates);
  std::map<m2::PointD, JunctionCandidateInfo> pointCandidates;
  auto const selectCandidates = [&m2Point, &pointCandidates, this](FeatureType & ft)
  {
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    if (ft.GetGeomType() != feature::GeomType::Line || !routing::IsRoad(feature::TypesHolder(ft)))
      return;

    for (auto i : {size_t(0), ft.GetPointsCount() - 1})
    {
      double weight = mercator::DistanceOnEarth(m2Point, ft.GetPoint(i));

      if (weight > m_junctionRadius)
        continue;

      weight *= GetHighwayTypePenalty(routing::CarModel::AllLimitsInstance().GetHighwayType(feature::TypesHolder(ft)),
                                      m_message.value().m_location.value().m_roadClass,
                                      m_message.value().m_location.value().m_ramps);

      auto refs = ftypes::GetRoadShieldsNames(ft);
      weight *= GetRoadRefPenalty(refs);

      /*
       * Store candidate point and weight (unless we already have a lower weight).
       * These are points read directly from the map, so we should be able to work with true matches
       * (according to tests, near-matches are rare and the one we examined was close to the
       * tolerance limit, so it could have been accidental).
       */
      auto it = pointCandidates.find(ft.GetPoint(i));
      if (it == pointCandidates.end())
        it = pointCandidates.insert(std::make_pair(ft.GetPoint(i), JunctionCandidateInfo(weight))).first;
      else if (weight < it->second.m_weight)
        it->second.m_weight = weight;

      // check oneway attribute and increase appropriate segment count
      if (!ftypes::IsOneWayChecker::Instance()(ft))
        it->second.m_twoWaySegments++;
      else if (i == 0)
        it->second.m_segmentsOut++;
      else
        it->second.m_segmentsIn++;
    }
  };

  m_dataSource.ForEachInRect(selectCandidates, mercator::RectByCenterXYAndSizeInMeters(m2Point, m_junctionRadius),
                             scales::GetUpperScale());

  /*
   * Cycle through point candidates and see if they are really junctions. A point is a junction if
   * it can be left through more than one segment, other than the one through which it was reached,
   * or reached through more than one segment, other than the one through which it will be left.
   * Junctions are added to `junctions`, other points are skipped.
   * Bug: may fail to catch duplicate ways at MWM boundaries
   */
  for (auto & [candidatePoint, candidateInfo] : pointCandidates)
  {
    if (candidateInfo.m_segmentsIn > 0)
      candidateInfo.m_segmentsIn--;
    else if (candidateInfo.m_twoWaySegments > 0)
      candidateInfo.m_twoWaySegments--;

    if (candidateInfo.m_segmentsOut > 0)
      candidateInfo.m_segmentsOut--;
    else if (candidateInfo.m_twoWaySegments > 0)
      candidateInfo.m_twoWaySegments--;

    if ((candidateInfo.m_segmentsIn > 0)
        || (candidateInfo.m_segmentsOut > 0)
        || (candidateInfo.m_twoWaySegments > 0))
      junctions.insert(std::make_pair(candidatePoint, candidateInfo.m_weight));
  }
}

traffxml::RoadClass GetRoadClass(routing::HighwayType highwayType)
{
  switch(highwayType)
  {
  /*
   * Parallel carriageways may be tagged as link, hence consider them equivalent to the underlying
   * highway type.
   */
  case routing::HighwayType::HighwayMotorway:
  case routing::HighwayType::HighwayMotorwayLink:
    return traffxml::RoadClass::Motorway;
  case routing::HighwayType::HighwayTrunk:
  case routing::HighwayType::HighwayTrunkLink:
    return traffxml::RoadClass::Trunk;
  case routing::HighwayType::HighwayPrimary:
  case routing::HighwayType::HighwayPrimaryLink:
    return traffxml::RoadClass::Primary;
  case routing::HighwayType::HighwaySecondary:
  case routing::HighwayType::HighwaySecondaryLink:
    return traffxml::RoadClass::Secondary;
  case routing::HighwayType::HighwayTertiary:
  case routing::HighwayType::HighwayTertiaryLink:
    return traffxml::RoadClass::Tertiary;
  default:
    return traffxml::RoadClass::Other;
  }
}

/**
 * @brief Returns the penalty factor for road class match/mismatch.
 *
 * If `lhs` and `rhs` are identical, the penalty factor is 1 (no penalty). If they are adjacent road
 * classes (e.g. trunk and primary), the penalty factor is `kReducedAttributePenalty`, in all other
 * cases it is `kAttributePenalty`.
 *
 * @param lhs
 * @param rhs
 * @return The penalty factor, see above.
 */
double GetRoadClassPenalty(traffxml::RoadClass lhs, traffxml::RoadClass rhs)
{
  if (lhs == rhs)
    return 1;
  switch (lhs)
  {
  case traffxml::RoadClass::Motorway:
    if (rhs == traffxml::RoadClass::Trunk)
      return kReducedAttributePenalty;
    else
      return kAttributePenalty;
  case traffxml::RoadClass::Trunk:
    if (rhs == traffxml::RoadClass::Motorway || rhs == traffxml::RoadClass::Primary)
      return kReducedAttributePenalty;
    else
      return kAttributePenalty;
  case traffxml::RoadClass::Primary:
    if (rhs == traffxml::RoadClass::Trunk || rhs == traffxml::RoadClass::Secondary)
      return kReducedAttributePenalty;
    else
      return kAttributePenalty;
  case traffxml::RoadClass::Secondary:
    if (rhs == traffxml::RoadClass::Primary || rhs == traffxml::RoadClass::Tertiary)
      return kReducedAttributePenalty;
    else
      return kAttributePenalty;
  case traffxml::RoadClass::Tertiary:
    if (rhs == traffxml::RoadClass::Secondary || rhs == traffxml::RoadClass::Other)
      return kReducedAttributePenalty;
    else
      return kAttributePenalty;
  case traffxml::RoadClass::Other:
    if (rhs == traffxml::RoadClass::Tertiary)
      return kReducedAttributePenalty;
    else
      return kAttributePenalty;
  default:
    UNREACHABLE();
  }
}

bool IsRamp(routing::HighwayType highwayType)
{
  switch(highwayType)
  {
  case routing::HighwayType::HighwayMotorwayLink:
  case routing::HighwayType::HighwayTrunkLink:
  case routing::HighwayType::HighwayPrimaryLink:
  case routing::HighwayType::HighwaySecondaryLink:
  case routing::HighwayType::HighwayTertiaryLink:
    return true;
  default:
    return false;
  }
}

std::vector<std::string> ParseRef(std::string const & ref)
{
  std::vector<std::string> res;
  std::string curr = "";
  RefParserState state = RefParserState::Whitespace;

  for (size_t i = 0; i < ref.size(); i++)
  {
    // TODO this list of delimiters might not be exhaustive
    if ((ref[i] <= 0x20) || (ref[i] == ',') || (ref[i] == '-') || (ref[i] == '.') || (ref[i] == '/'))
    {
      // whitespace
      if (state != RefParserState::Whitespace)
      {
        if (state == RefParserState::Alpha)
          boost::to_lower(curr);
        res.push_back(curr);
        curr = "";
      }
      state = RefParserState::Whitespace;
    }
    /*
     * TODO adapt this to other number systems as well.
     * Roman numerals (or any use of letters as numbers) are a stupid idea. If they are at least
     * properly delimited, they are treated as a letter group, which sort of works for comparison.
     * However, `IVbis` will be treated as one group, whereas `IV bis` will be treated as two.
     */
    else if ((ref[i] >= '0') && (ref[i] <= '9'))
    {
      // numeric
      if (state == RefParserState::Alpha)
      {
        boost::to_lower(curr);
        res.push_back(curr);
        curr = "";
      }
      curr += ref[i];
      state = RefParserState::Numeric;
    }
    // anything that is not a delimiter or a digit (as per the above rules) is considered a letter
    else
    {
      // alpha
      if (state == RefParserState::Numeric)
      {
        res.push_back(curr);
        curr = "";
      }
      curr += ref[i];
      state = RefParserState::Alpha;
    }
  }
  if (!curr.empty())
    res.push_back(curr);
  return res;
}

void TruncateStart(std::vector<routing::RouteSegment> & rsegments,
                   routing::Checkpoints const & checkpoints,
                   size_t & start, double & startSaving, double const startWeight,
                   std::map<m2::PointD, double> const & junctions)
{
  if (rsegments.empty())
    return;

  for (size_t i = 0; i < rsegments.size(); i++)
  {
    double newStartSaving = 0;
    /*
     * Examine end point of segment: for a junction, take weight from table; else calculate it as
     * direct distance multiplied with offroad penalty; calculate saving based on that.
     */
    auto it = junctions.find(rsegments[i].GetJunction().GetPoint());
    if (it != junctions.end())
      newStartSaving = rsegments[i].GetTimeFromBeginningSec() - it->second;
    else
    {
      bool matched = false;
      // TODO this is likely an inefficient way to return near-matches
      for (auto & [point, weight] : junctions)
        if (rsegments[i].GetJunction().GetPoint().EqualDxDy(point, kMwmPointAccuracy))
        {
          newStartSaving = rsegments[i].GetTimeFromBeginningSec() - weight;
          matched = true;
          break;
        }
      if (!matched)
        newStartSaving = rsegments[i].GetTimeFromBeginningSec()
            - (mercator::DistanceOnEarth(checkpoints.GetStart(), rsegments[i].GetJunction().GetPoint())
                * kOffroadPenalty);
    }
    if (newStartSaving > startSaving)
    {
      start = i + 1; // add 1 because we are ditching this segment and keeping the next one
      startSaving = newStartSaving;
    }
  }
  /*
   * The router may return a route that starts and ends with a partial segment. In decoder mode,
   * we don’t allow starting or ending a segment at an intermediate point but use the nearest
   * segment endpoint instead. However, this may leave us with a zero-length segment, which we
   * need to remove so we don’t overshoot the reference point.
   */
  // TODO not if we have an `at` point
  if ((start == 0) && (rsegments.size() > 1)
      && (rsegments[0].GetTimeFromBeginningSec() == startWeight))
      start = 1;
}

void TruncateEnd(std::vector<routing::RouteSegment> & rsegments,
                 routing::Checkpoints const & checkpoints,
                 size_t & end, double & endSaving, double const endWeight,
                 std::map<m2::PointD, double> const & junctions)
{
  for (size_t i = 0; i < rsegments.size(); i++)
  {
    double newEndSaving = 0;
    /*
     * Examine end point of segment: for a junction, take weight from table; else calculate it as
     * direct distance multiplied with offroad penalty; calculate saving based on that.
     */
    auto it = junctions.find(rsegments[i].GetJunction().GetPoint());
    if (it != junctions.end())
      newEndSaving = endWeight - rsegments[i].GetTimeFromBeginningSec() - it->second;
    else
    {
      bool matched = false;
      // TODO this is likely an inefficient way to return near-matches
      for (auto & [point, weight] : junctions)
        if (rsegments[i].GetJunction().GetPoint().EqualDxDy(point, kMwmPointAccuracy))
        {
          newEndSaving = endWeight - rsegments[i].GetTimeFromBeginningSec() - weight;
          matched = true;
          break;
        }
      if (!matched)
        newEndSaving = endWeight - rsegments[i].GetTimeFromBeginningSec()
            - (mercator::DistanceOnEarth(rsegments[i].GetJunction().GetPoint(), checkpoints.GetFinish())
                * kOffroadPenalty);
    }
    if (newEndSaving > endSaving)
    {
      end = i;
      endSaving = newEndSaving;
    }
  }
  /*
   * The router may return a route that starts and ends with a partial segment. In decoder mode,
   * we don’t allow starting or ending a segment at an intermediate point but use the nearest
   * segment endpoint instead. However, this may leave us with a zero-length segment, which we
   * need to remove so we don’t overshoot the reference point.
   */
  // TODO not if we have an `at` point
  if ((end == (rsegments.size() - 1)) && (rsegments.size() > 1)
      && (rsegments[rsegments.size() - 1].GetTimeFromBeginningSec()
          == rsegments[rsegments.size() - 2].GetTimeFromBeginningSec()))
      end--;
}
}  // namespace traffxml
