#import "MWMWatchMapExtractor.h"

#import "MWMRoutePoint.h"
#import "MWMRouter.h"

#import <CoreApi/Framework.h>

#include "map/bookmark_manager.hpp"

#include "kml/type_utils.hpp"

#include "routing/following_info.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_decl.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

// Keep in sync with WatchMapContext.swift.
uint8_t constexpr kKindMajorRoad = 0;
uint8_t constexpr kKindMinorRoad = 1;
uint8_t constexpr kKindPath = 2;
uint8_t constexpr kKindRail = 3;
uint8_t constexpr kKindWaterLine = 4;
uint8_t constexpr kKindWaterArea = 5;
uint8_t constexpr kKindNone = 0xFF;

uint16_t constexpr kFormatVersion = 1;

/// Geometry scale: enough for residential streets and footpaths while the
/// stored geometry is already simplified compared to the full detail level.
int constexpr kScale = 15;
/// The corridor is the union of squares of this half-size centered on route
/// points sampled this far apart.
double constexpr kSampleSpacingMeters = 400.0;
double constexpr kCorridorHalfSizeMeters = 500.0;
/// Budgets keep the transferred blob in the hundreds-of-KB range.
size_t constexpr kMaxTotalPoints = 40000;
size_t constexpr kMaxFeaturePoints = 512;

struct PackedFeature
{
  uint8_t m_kind = kKindNone;
  // (lat, lon) pairs; for kKindWaterArea consecutive triples form triangles.
  std::vector<std::pair<float, float>> m_points;
};

class TypeClassifier
{
public:
  TypeClassifier()
  {
    auto const & c = classif();
    m_highway = c.GetTypeByPath({"highway"});
    for (char const * t : {"motorway", "trunk", "primary", "secondary",
                           "motorway_link", "trunk_link", "primary_link", "secondary_link"})
      m_major.insert(c.GetTypeByPath({"highway", t}));
    for (char const * t : {"footway", "path", "cycleway", "steps", "pedestrian", "track"})
      m_path.insert(c.GetTypeByPath({"highway", t}));
    for (char const * t : {"river", "stream", "canal"})
      m_waterLine.insert(c.GetTypeByPath({"waterway", t}));
    m_rail = c.GetTypeByPath({"railway", "rail"});
    m_waterArea = c.GetTypeByPath({"natural", "water"});
  }

  uint8_t Kind(FeatureType & ft) const
  {
    feature::TypesHolder const holder(ft);
    for (uint32_t t : holder)
    {
      uint32_t two = t;
      ftype::TruncValue(two, 2);
      if (m_major.count(two) != 0)
        return kKindMajorRoad;
      if (m_path.count(two) != 0)
        return kKindPath;
      if (m_waterLine.count(two) != 0)
        return kKindWaterLine;
      if (two == m_rail)
        return kKindRail;
      if (two == m_waterArea)
        return kKindWaterArea;
      uint32_t one = t;
      ftype::TruncValue(one, 1);
      if (one == m_highway)
        return kKindMinorRoad;
    }
    return kKindNone;
  }

private:
  uint32_t m_highway = 0;
  uint32_t m_rail = 0;
  uint32_t m_waterArea = 0;
  std::unordered_set<uint32_t> m_major;
  std::unordered_set<uint32_t> m_path;
  std::unordered_set<uint32_t> m_waterLine;
};

void AppendLatLon(std::vector<std::pair<float, float>> & out, m2::PointD const & pt)
{
  out.emplace_back(static_cast<float>(mercator::YToLat(pt.y)),
                   static_cast<float>(mercator::XToLon(pt.x)));
}

/// Keeps every strideth point but never drops the last one.
void Decimate(std::vector<std::pair<float, float>> & points, size_t maxPoints)
{
  if (points.size() <= maxPoints)
    return;
  size_t const stride = (points.size() + maxPoints - 1) / maxPoints;
  std::vector<std::pair<float, float>> kept;
  kept.reserve(maxPoints + 1);
  for (size_t i = 0; i < points.size(); i += stride)
    kept.push_back(points[i]);
  if (kept.back() != points.back())
    kept.push_back(points.back());
  points = std::move(kept);
}

NSData * PackFeatures(std::vector<PackedFeature> const & features, NSTimeInterval timestamp)
{
  NSMutableData * data = [NSMutableData data];
  [data appendBytes:"CMWC" length:4];
  uint16_t const version = kFormatVersion;
  uint16_t const pad = 0;
  uint32_t const count = static_cast<uint32_t>(features.size());
  double const ts = timestamp;
  [data appendBytes:&version length:sizeof(version)];
  [data appendBytes:&pad length:sizeof(pad)];
  [data appendBytes:&count length:sizeof(count)];
  [data appendBytes:&ts length:sizeof(ts)];
  for (auto const & f : features)
  {
    uint8_t const kind = f.m_kind;
    uint8_t const zero = 0;
    uint16_t const pointCount = static_cast<uint16_t>(f.m_points.size());
    [data appendBytes:&kind length:sizeof(kind)];
    [data appendBytes:&zero length:sizeof(zero)];
    [data appendBytes:&pointCount length:sizeof(pointCount)];
    [data appendBytes:f.m_points.data() length:f.m_points.size() * 2 * sizeof(float)];
  }
  return data;
}

NSData * ExtractCorridor(std::vector<m2::PointD> const & polyline, NSTimeInterval timestamp)
{
  TypeClassifier const classifier;

  std::vector<m2::RectD> rects;
  m2::PointD lastSample = polyline.front();
  rects.push_back(mercator::RectByCenterXYAndSizeInMeters(lastSample, kCorridorHalfSizeMeters));
  for (auto const & pt : polyline)
  {
    if (mercator::DistanceOnEarth(lastSample, pt) < kSampleSpacingMeters)
      continue;
    lastSample = pt;
    rects.push_back(mercator::RectByCenterXYAndSizeInMeters(pt, kCorridorHalfSizeMeters));
  }

  std::set<FeatureID> seen;
  std::vector<PackedFeature> features;
  size_t totalPoints = 0;
  auto const & dataSource = GetFramework().GetDataSource();
  for (auto const & rect : rects)
  {
    dataSource.ForEachInRect([&](FeatureType & ft)
    {
      if (!seen.insert(ft.GetID()).second)
        return;
      uint8_t const kind = classifier.Kind(ft);
      if (kind == kKindNone)
        return;

      PackedFeature packed;
      packed.m_kind = kind;
      auto const geomType = ft.GetGeomType();
      if (kind == kKindWaterArea)
      {
        if (geomType != feature::GeomType::Area)
          return;
        ft.ForEachTriangle([&](m2::PointD const & a, m2::PointD const & b, m2::PointD const & c)
        {
          AppendLatLon(packed.m_points, a);
          AppendLatLon(packed.m_points, b);
          AppendLatLon(packed.m_points, c);
        }, kScale);
        // Triangles must stay in whole triples, so no decimation here.
        if (packed.m_points.size() > kMaxFeaturePoints * 3)
          packed.m_points.resize(kMaxFeaturePoints * 3);
      }
      else
      {
        if (geomType != feature::GeomType::Line)
          return;
        ft.ForEachPoint([&](m2::PointD const & pt) { AppendLatLon(packed.m_points, pt); }, kScale);
        Decimate(packed.m_points, kMaxFeaturePoints);
      }
      if (packed.m_points.size() < 2)
        return;
      totalPoints += packed.m_points.size();
      features.push_back(std::move(packed));
    }, rect, kScale);
  }

  // Trim the least important kinds first when over budget.
  for (uint8_t kind : {kKindPath, kKindMinorRoad, kKindRail})
  {
    if (totalPoints <= kMaxTotalPoints)
      break;
    for (auto it = features.begin(); it != features.end() && totalPoints > kMaxTotalPoints;)
    {
      if (it->m_kind == kind)
      {
        totalPoints -= it->m_points.size();
        it = features.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  NSData * data = PackFeatures(features, timestamp);
  LOG(LINFO, ("WatchMapExtractor: corridor rects:", rects.size(), "features:", features.size(),
              "points:", totalPoints, "bytes:", [data length]));
  return data;
}

}  // namespace

@implementation MWMWatchMapExtractor

+ (void)extractCorridorWithTimestamp:(NSTimeInterval)timestamp
                          completion:(void (^)(NSData *_Nullable))completion {
  auto const &rm = GetFramework().GetRoutingManager();
  if (!rm.IsRouteValid()) {
    completion(nil);
    return;
  }
  auto const polyline =
    std::make_shared<std::vector<m2::PointD>>(rm.GetRoutePolyline().GetPolyline().GetPoints());
  if (polyline->empty()) {
    completion(nil);
    return;
  }
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
    NSData *data = ExtractCorridor(*polyline, timestamp);
    dispatch_async(dispatch_get_main_queue(), ^{
      completion(data);
    });
  });
}

+ (nullable NSData *)routeAltitudesWithMaxPoints:(NSUInteger)maxPoints
                                          ascent:(double *)totalAscent
                                         descent:(double *)totalDescent {
  auto &rm = GetFramework().GetRoutingManager();
  if (!rm.HasRouteAltitude())
    return nil;
  RoutingManager::DistanceAltitude distanceAltitude;
  if (!rm.GetRouteAltitudesAndDistancesM(distanceAltitude))
    return nil;
  distanceAltitude.Simplify();
  size_t const size = distanceAltitude.GetSize();
  if (size < 2)
    return nil;

  uint32_t ascentM = 0, descentM = 0;
  distanceAltitude.CalculateAscentDescent(ascentM, descentM);
  if (totalAscent)
    *totalAscent = ascentM;
  if (totalDescent)
    *totalDescent = descentM;

  size_t const stride = std::max<size_t>(1, (size + maxPoints - 1) / std::max<NSUInteger>(maxPoints, 2));
  NSMutableData *data = [NSMutableData data];
  auto append = [&](size_t i) {
    float const pair[2] = {static_cast<float>(distanceAltitude.m_distances[i]),
                           static_cast<float>(distanceAltitude.m_altitudes[i])};
    [data appendBytes:pair length:sizeof(pair)];
  };
  size_t last = 0;
  for (size_t i = 0; i < size; i += stride) {
    append(i);
    last = i;
  }
  if (last != size - 1)
    append(size - 1);
  return data;
}

+ (NSArray<NSDictionary<NSString *, id> *> *)watchDestinationsWithLimit:(NSUInteger)limit {
  auto const &bookmarkManager = GetFramework().GetBookmarkManager();
  NSMutableArray<NSDictionary<NSString *, id> *> *result = [NSMutableArray array];
  for (auto const groupId : bookmarkManager.GetSortedBmGroupIdList()) {
    for (auto const markId : bookmarkManager.GetUserMarkIds(groupId)) {
      if (result.count >= limit)
        return result;
      Bookmark const *bookmark = bookmarkManager.GetBookmark(markId);
      if (bookmark == nullptr)
        continue;
      auto const latLon = mercator::ToLatLon(bookmark->GetPivot());
      [result addObject:@{
        @"n": @(bookmark->GetPreferredName().c_str()),
        @"la": @(latLon.m_lat),
        @"lo": @(latLon.m_lon),
      }];
    }
  }
  return result;
}

+ (void)buildRouteToLatitude:(double)latitude longitude:(double)longitude name:(NSString *)name {
  if ([MWMRouter isRoutingActive])
    [MWMRouter stopRouting];
  CGPoint const mercatorPoint = CGPointMake(mercator::LonToX(longitude), mercator::LatToY(latitude));
  MWMRoutePoint *finish = [[MWMRoutePoint alloc] initWithCGPoint:mercatorPoint
                                                           title:name.length > 0 ? name : @""
                                                        subtitle:@""
                                                            type:MWMRoutePointTypeFinish
                                               intermediateIndex:0];
  [MWMRouter buildToPoint:finish bestRouter:NO];
}

+ (void)addBookmarkAtLatitude:(double)latitude longitude:(double)longitude name:(NSString *)name {
  auto &bookmarkManager = GetFramework().GetBookmarkManager();
  kml::BookmarkData data;
  kml::SetDefaultStr(data.m_name, name.UTF8String);
  data.m_point = mercator::FromLatLon(latitude, longitude);
  bookmarkManager.GetEditSession().CreateBookmark(std::move(data), bookmarkManager.LastEditedBMCategory());
}

+ (nullable NSDictionary<NSString *, id> *)currentTurnInfo {
  auto const &rm = GetFramework().GetRoutingManager();
  if (!rm.IsRoutingActive() || !rm.IsRouteValid())
    return nil;
  routing::FollowingInfo info;
  rm.GetRouteFollowingInfo(info);
  if (!info.m_distToTurn.IsValid())
    return nil;

  using routing::turns::CarDirection;
  using routing::turns::PedestrianDirection;
  NSString *symbol;
  switch (info.m_turn) {
    case CarDirection::TurnRight:
    case CarDirection::TurnSharpRight: symbol = @"arrow.turn.up.right"; break;
    case CarDirection::TurnSlightRight: symbol = @"arrow.up.right"; break;
    case CarDirection::TurnLeft:
    case CarDirection::TurnSharpLeft: symbol = @"arrow.turn.up.left"; break;
    case CarDirection::TurnSlightLeft: symbol = @"arrow.up.left"; break;
    case CarDirection::UTurnLeft: symbol = @"arrow.uturn.left"; break;
    case CarDirection::UTurnRight: symbol = @"arrow.uturn.right"; break;
    case CarDirection::EnterRoundAbout:
    case CarDirection::StayOnRoundAbout:
    case CarDirection::LeaveRoundAbout: symbol = @"arrow.triangle.turn.up.right.circle"; break;
    case CarDirection::ReachedYourDestination: symbol = @"flag.checkered"; break;
    case CarDirection::GoStraight: symbol = @"arrow.up"; break;
    default: symbol = nil; break;
  }
  if (symbol == nil) {
    switch (info.m_pedestrianTurn) {
      case PedestrianDirection::TurnRight: symbol = @"arrow.turn.up.right"; break;
      case PedestrianDirection::TurnLeft: symbol = @"arrow.turn.up.left"; break;
      case PedestrianDirection::ReachedYourDestination: symbol = @"flag.checkered"; break;
      default: symbol = @"arrow.up"; break;
    }
  }
  // Keys must stay in sync with WatchTurnInfo in WatchRoutePayload.swift.
  return @{
    @"tSym": symbol,
    @"tDist": @(info.m_distToTurn.ToString().c_str()),
    @"tStreet": @(info.m_nextStreetName.c_str()),
    @"ts": NSDate.date,
  };
}

@end
