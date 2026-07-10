#import "MWMLocationManager.h"
#import "MWMNavigationDashboardEntity.h"
#import "MWMNavigationDashboardManager+Entity.h"
#import "MWMRouter.h"
#import "MWMRouterTransitStepInfo.h"
#import "SwiftBridge.h"

#import <AudioToolbox/AudioServices.h>
#import <CoreApi/Framework.h>
#import <CoreApi/DurationFormatter.h>

#include "routing/following_info.hpp"
#include "routing/lanes/lane_info.hpp"
#include "routing/turns.hpp"

#include "indexer/road_shields_parser.hpp"

#include "map/routing_manager.hpp"

#include "platform/location.hpp"

#include "geometry/distance_on_sphere.hpp"

namespace {
UIImage * image(routing::turns::CarDirection t, bool isNextTurn) {
  if (![MWMLocationManager lastLocation])
    return nil;

  using namespace routing::turns;
  NSString * imageName;
  switch (t) {
    case CarDirection::ExitHighwayToRight: imageName = @"ic_exit_highway_to_right"; break;
    case CarDirection::TurnSlightRight: imageName = @"slight_right"; break;
    case CarDirection::TurnRight: imageName = @"simple_right"; break;
    case CarDirection::TurnSharpRight: imageName = @"sharp_right"; break;
    case CarDirection::ExitHighwayToLeft: imageName = @"ic_exit_highway_to_left"; break;
    case CarDirection::TurnSlightLeft: imageName = @"slight_left"; break;
    case CarDirection::TurnLeft: imageName = @"simple_left"; break;
    case CarDirection::TurnSharpLeft: imageName = @"sharp_left"; break;
    case CarDirection::UTurnLeft: imageName = @"uturn_left"; break;
    case CarDirection::UTurnRight: imageName = @"uturn_right"; break;
    case CarDirection::ReachedYourDestination: imageName = @"finish_point"; break;
    case CarDirection::LeaveRoundAbout:
    case CarDirection::EnterRoundAbout: imageName = @"round"; break;
    case CarDirection::GoStraight: imageName = @"straight"; break;
    case CarDirection::StartAtEndOfStreet:
    case CarDirection::StayOnRoundAbout:
    case CarDirection::Count:
    case CarDirection::None: imageName = isNextTurn ? nil : @"straight"; break;
  }
  if (!imageName)
    return nil;
  return [UIImage imageNamed:isNextTurn ? [imageName stringByAppendingString:@"_then"] : imageName];
}

UIImage * image(routing::turns::PedestrianDirection t) {
  if (![MWMLocationManager lastLocation])
    return nil;

  using namespace routing::turns;
  NSString * imageName;
  switch (t) {
    case PedestrianDirection::TurnRight: imageName = @"simple_right"; break;
    case PedestrianDirection::TurnLeft: imageName = @"simple_left"; break;
    case PedestrianDirection::ReachedYourDestination: imageName = @"finish_point"; break;
    case PedestrianDirection::GoStraight:
    case PedestrianDirection::Count:
    case PedestrianDirection::None: imageName = @"straight"; break;
  }
  if (!imageName)
    return nil;
  return [UIImage imageNamed:imageName];
}

NSArray<MWMRouterTransitStepInfo *> *buildRouteTransitSteps(NSArray<MWMRoutePoint *> *points) {
  // Generate step info in format: (Segment 1 distance) (1) (Segment 2 distance) (2) ... (n-1) (Segment N distance).
  NSMutableArray<MWMRouterTransitStepInfo *> * steps = [NSMutableArray arrayWithCapacity:[points count] * 2 - 1];
  auto const numPoints = [points count];
  for (int i = 0; i < numPoints - 1; i++) {
    MWMRoutePoint* segmentStart = points[i];
    MWMRoutePoint* segmentEnd = points[i + 1];
    auto const distance = platform::Distance::CreateFormatted(
        ms::DistanceOnEarth(segmentStart.latitude, segmentStart.longitude, segmentEnd.latitude, segmentEnd.longitude));

    MWMRouterTransitStepInfo* segmentInfo = [[MWMRouterTransitStepInfo alloc] init];
    segmentInfo.type = MWMRouterTransitTypeRuler;
    segmentInfo.distance = @(distance.GetDistanceString().c_str());
    segmentInfo.distanceUnits = @(distance.GetUnitsString().c_str());
    steps[i * 2] = segmentInfo;

    if (i < numPoints - 2) {
      MWMRouterTransitStepInfo* stopInfo = [[MWMRouterTransitStepInfo alloc] init];
      stopInfo.type = MWMRouterTransitTypeIntermediatePoint;
      stopInfo.intermediateIndex = i;
      steps[i * 2 + 1] = stopInfo;
    }
  }

  return steps;
}

NSArray<MWMLaneInfo *> *buildLanes(routing::turns::lanes::LanesInfo const & info) {
  NSMutableArray<MWMLaneInfo *> * lanes = [NSMutableArray arrayWithCapacity:info.size()];
  for (auto const & lane : info) {
    auto const activeWays = lane.laneWays.GetActiveLaneWays();
    NSMutableArray<NSNumber *> * laneWays = [NSMutableArray arrayWithCapacity:activeWays.size()];
    for (auto const way : activeWays)
      [laneWays addObject:@(static_cast<uint8_t>(way))];
    [lanes addObject:[[MWMLaneInfo alloc] initWithLaneWays:laneWays
                                            recommendedWay:static_cast<uint8_t>(lane.recommendedWay)]];
  }
  return lanes;
}

// Collapse the full ftypes::RoadShieldType set into the few styles the UI renders.
// Keep in sync with android/.../roadshield/RoadShieldType.cpp and the MWMRoadShieldType enum.
MWMRoadShieldType roadShieldType(ftypes::RoadShieldType type) {
  using enum ftypes::RoadShieldType;
  switch (type) {
    case Default:
    case Hidden:
    case Count:
    case Generic_White:
    case Generic_White_Bordered:
    case Generic_Pill_White:
    case Generic_Pill_White_Bordered:
    // No dedicated grey style; fall back to the neutral white shield.
    case Generic_Grey:
    case Generic_Grey_Bordered:
    // Brazilian federal (BR) and state road shields are white.
    case Brazil_National:
    case Brazil_State: return MWMRoadShieldTypeGenericWhite;
    case Generic_Green:
    case Generic_Green_Bordered:
    case Generic_Pill_Green:
    case Generic_Pill_Green_Bordered:
    case Highway_Hexagon_Green:
    case Italy_Autostrada:
    case Hungary_Green: return MWMRoadShieldTypeGenericGreen;
    case Generic_Blue:
    case Generic_Blue_Bordered:
    case Generic_Pill_Blue:
    case Generic_Pill_Blue_Bordered:
    case Highway_Hexagon_Blue:
    case Highway_Hexagon_Turkey:
    case UY_National:
    case Hungary_Blue:
    case Argentina_RN:
    case Bolivia_Fundamental: return MWMRoadShieldTypeGenericBlue;
    case Generic_Red:
    case Generic_Red_Bordered:
    case Generic_Pill_Red:
    case Generic_Pill_Red_Bordered:
    case Highway_Hexagon_Red: return MWMRoadShieldTypeGenericRed;
    case Generic_Orange:
    case Generic_Orange_Bordered:
    case Generic_Pill_Orange:
    case Generic_Pill_Orange_Bordered: return MWMRoadShieldTypeGenericOrange;
    case US_Interstate: return MWMRoadShieldTypeUsInterstate;
    case US_Highway: return MWMRoadShieldTypeUsHighway;
    case UK_Highway: return MWMRoadShieldTypeUkHighway;
  }
  return MWMRoadShieldTypeGenericWhite;
}

NSArray<MWMRoadShield *> *buildRoadShields(ftypes::RoadShieldsSetT const & shields) {
  NSMutableArray<MWMRoadShield *> * result = [NSMutableArray arrayWithCapacity:shields.size()];
  for (auto const & shield : shields) {
    // GetShieldText() is the text drawn inside the box (with any prefix a country symbol would carry,
    // e.g. Brazilian "BR-116"); m_additionalText (e.g. US "East") is drawn next to the shield.
    NSString * text = @(shield.GetShieldText().c_str());
    NSString * additionalText = shield.m_additionalText.empty() ? nil : @(shield.m_additionalText.c_str());
    [result addObject:[[MWMRoadShield alloc] initWithType:roadShieldType(shield.m_type)
                                                     text:text
                                           additionalText:additionalText]];
  }
  return result;
}

MWMRoadShieldInfo *buildRoadShieldInfo(routing::FollowingInfo::RoadShieldInfo const & info) {
  if (info.m_targetRoadShields.empty() && info.m_junctionRoadShields.empty())
    return nil;
  return [[MWMRoadShieldInfo alloc] initWithTargetRoadShields:buildRoadShields(info.m_targetRoadShields)
                                         junctionRoadShields:buildRoadShields(info.m_junctionRoadShields)];
}
}  // namespace

@interface MWMNavigationDashboardEntity ()

@property(copy, nonatomic, readwrite) NSArray<MWMRouterTransitStepInfo *> * transitSteps;
@property(copy, nonatomic, readwrite) NSArray<MWMLaneInfo *> * lanes;
@property(copy, nonatomic, readwrite) NSString * distanceToTurn;
@property(copy, nonatomic, readwrite) NSString * streetName;
@property(copy, nonatomic, readwrite) NSString * nextRoadName;
@property(copy, nonatomic, readwrite) NSString * nextRoadRef;
@property(copy, nonatomic, readwrite) NSString * nextJunctionRef;
@property(copy, nonatomic, readwrite) NSString * nextDestinationRef;
@property(copy, nonatomic, readwrite) NSString * nextDestination;
@property(nonatomic, readwrite) BOOL nextIsLink;
@property(nonatomic, readwrite) BOOL isLeftHandTraffic;
@property(nonatomic, readwrite) MWMRoadShieldInfo * nextRoadShields;
@property(copy, nonatomic, readwrite) NSString * targetDistance;
@property(copy, nonatomic, readwrite) NSString * targetUnits;
@property(copy, nonatomic, readwrite) NSString * turnUnits;
@property(nonatomic, readwrite) double speedLimitMps;
@property(nonatomic, readwrite) BOOL isValid;
@property(nonatomic, readwrite) CGFloat progress;
@property(nonatomic, readwrite) NSUInteger roundExitNumber;
@property(nonatomic, readwrite) NSUInteger timeToTarget;
@property(nonatomic, readwrite) UIImage * nextTurnImage;
@property(nonatomic, readwrite) UIImage * turnImage;
@property(nonatomic, readwrite) BOOL showEta;
@property(nonatomic, readwrite) BOOL isWalk;

@end

@implementation MWMNavigationDashboardEntity

- (NSString *)arrival
{
  auto arrivalDate = [[NSDate date] dateByAddingTimeInterval:self.timeToTarget];
  return [DateTimeFormatter dateStringFrom:arrivalDate
                                 dateStyle:NSDateFormatterNoStyle
                                 timeStyle:NSDateFormatterShortStyle];
}

+ (NSAttributedString *)estimateDot
{
  auto attributes = @{
    NSForegroundColorAttributeName: [UIColor blackSecondaryText],
    NSFontAttributeName: [UIFont medium17]
  };
  return [[NSAttributedString alloc] initWithString:@" • " attributes:attributes];
}

- (NSAttributedString *)estimate {
  NSDictionary * primaryAttributes = @{NSForegroundColorAttributeName: [UIColor blackPrimaryText], NSFontAttributeName: [UIFont medium17]};
  NSDictionary * secondaryAttributes = @{NSForegroundColorAttributeName: [UIColor blackSecondaryText], NSFontAttributeName: [UIFont medium17]};

  auto result = [[NSMutableAttributedString alloc] initWithString:@""];
  if (self.showEta) {
    NSString * eta = [DurationFormatter durationStringFromTimeInterval:self.timeToTarget];
    [result appendAttributedString:[[NSMutableAttributedString alloc] initWithString:eta attributes:primaryAttributes]];
    [result appendAttributedString:MWMNavigationDashboardEntity.estimateDot];
  }

  if (self.isWalk) {
    UIFont * font = primaryAttributes[NSFontAttributeName];
    auto textAttachment = [[NSTextAttachment alloc] init];
    auto image = [UIImage imageNamed:@"ic_walk"];
    textAttachment.image = image;
    auto const height = font.lineHeight;
    auto const y = height - image.size.height;
    auto const width = image.size.width * height / image.size.height;
    textAttachment.bounds = CGRectIntegral({{0, y}, {width, height}});

    NSMutableAttributedString * attrStringWithImage =
    [NSAttributedString attributedStringWithAttachment:textAttachment].mutableCopy;
    [attrStringWithImage addAttributes:secondaryAttributes range:NSMakeRange(0, attrStringWithImage.length)];
    [result appendAttributedString:attrStringWithImage];
  }

  auto target = [NSString stringWithFormat:@"%@ %@", self.targetDistance, self.targetUnits];
  [result appendAttributedString:[[NSAttributedString alloc] initWithString:target attributes:secondaryAttributes]];

  return result;
}

- (NSAttributedString *)attributedInstructionForTextSize:(CGFloat)textSize textColor:(UIColor *)textColor {
  if (self.nextRoadName.length == 0 && self.nextRoadRef.length == 0 && self.nextJunctionRef.length == 0 &&
      self.nextDestinationRef.length == 0 && self.nextDestination.length == 0)
    return nil;
  return [MWMNavigationInstructionFormatter attributedInstructionWithNextStreet:self.streetName ?: @""
                                                                       roadName:self.nextRoadName ?: @""
                                                                        roadRef:self.nextRoadRef ?: @""
                                                                    junctionRef:self.nextJunctionRef ?: @""
                                                                 destinationRef:self.nextDestinationRef ?: @""
                                                                    destination:self.nextDestination ?: @""
                                                                         isLink:self.nextIsLink
                                                              isLeftHandTraffic:self.isLeftHandTraffic
                                                                        shields:self.nextRoadShields
                                                                       textSize:textSize
                                                                      textColor:textColor];
}

@end

@interface MWMRouterTransitStepInfo ()

- (instancetype)initWithStepInfo:(TransitStepInfo const &)info;

@end

@interface MWMNavigationDashboardManager ()

@property(copy, nonatomic) NSDictionary * etaAttributes;
@property(copy, nonatomic) NSDictionary * etaSecondaryAttributes;
@property(nonatomic) MWMNavigationDashboardEntity * entity;

- (void)onNavigationInfoUpdated;

@end

@implementation MWMNavigationDashboardManager (Entity)

- (void)updateFollowingInfo:(routing::FollowingInfo const &)info routePoints:(NSArray<MWMRoutePoint *> *)points type:(MWMRouterType)type {
  if ([MWMRouter isRouteFinished]) {
    [MWMRouter stopRouting];
    AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
    return;
  }

  if (auto entity = self.entity) {
    BOOL const showEta = (type != MWMRouterTypeRuler);
    
    entity.isValid = YES;
    entity.timeToTarget = info.m_time;
    entity.targetDistance = @(info.m_distToTarget.GetDistanceString().c_str());
    entity.targetUnits = @(info.m_distToTarget.GetUnitsString().c_str());
    entity.progress = info.m_completionPercent;
    entity.distanceToTurn = @(info.m_distToTurn.GetDistanceString().c_str());
    entity.turnUnits = @(info.m_distToTurn.GetUnitsString().c_str());
    entity.streetName = @(info.m_nextStreetName.c_str());
    entity.speedLimitMps = info.m_speedLimitMps;

    entity.isWalk = NO;
    entity.showEta = showEta;

    if (type == MWMRouterTypeRuler && [points count] > 2)
      entity.transitSteps = buildRouteTransitSteps(points);
    else
      entity.transitSteps = [[NSArray alloc] init];

    if (type == MWMRouterTypePedestrian) {
      entity.turnImage = image(info.m_pedestrianTurn);
      entity.lanes = @[];
    } else {
      using namespace routing::turns;
      CarDirection const turn = info.m_turn;
      entity.turnImage = image(turn, false);
      entity.nextTurnImage = image(info.m_nextTurn, true);
      BOOL const isRound = turn == CarDirection::EnterRoundAbout || turn == CarDirection::StayOnRoundAbout ||
                           turn == CarDirection::LeaveRoundAbout;
      if (isRound)
        entity.roundExitNumber = info.m_exitNum;
      else
        entity.roundExitNumber = 0;
      entity.lanes = buildLanes(info.m_lanes);

      entity.nextRoadName = @(info.m_nextName.c_str());
      entity.nextRoadRef = @(info.m_nextRef.c_str());
      entity.nextJunctionRef = @(info.m_nextJunctionRef.c_str());
      entity.nextDestinationRef = @(info.m_nextDestinationRef.c_str());
      entity.nextDestination = @(info.m_nextDestination.c_str());
      entity.nextIsLink = info.m_nextIsLink;
      entity.isLeftHandTraffic = info.m_isLeftHandTraffic;
      entity.nextRoadShields = buildRoadShieldInfo(info.m_nextStreetShields);

      NSArray<NSString *> * variants =
          [MWMNavigationInstructionFormatter instructionVariantsWithRoadName:entity.nextRoadName
                                                                     roadRef:entity.nextRoadRef
                                                                 junctionRef:entity.nextJunctionRef
                                                              destinationRef:entity.nextDestinationRef
                                                                 destination:entity.nextDestination
                                                                      isLink:info.m_nextIsLink];
      if (variants.firstObject.length != 0)
        entity.streetName = variants.firstObject;
    }
  }

  [self onNavigationInfoUpdated];
}

- (void)updateTransitInfo:(TransitRouteInfo const &)info {
  if (auto entity = self.entity) {
    entity.timeToTarget = info.m_totalTimeInSec;
    entity.targetDistance = @(info.m_totalPedestrianDistanceStr.c_str());
    entity.targetUnits = @(info.m_totalPedestrianUnitsSuffix.c_str());
    entity.isValid = YES;
    entity.isWalk = YES;
    entity.showEta = YES;
    NSMutableArray<MWMRouterTransitStepInfo *> * transitSteps = [NSMutableArray new];
    for (auto const &stepInfo : info.m_steps)
      [transitSteps addObject:[[MWMRouterTransitStepInfo alloc] initWithStepInfo:stepInfo]];
    entity.transitSteps = transitSteps;
  }
  [self onNavigationInfoUpdated];
}

@end
