#import "MWMLaneInfo+CPP.h"
#import "SwiftBridge.h"

NSArray<MWMLaneInfo *> * MWMBuildLaneInfos(routing::turns::lanes::LanesInfo const & lanes)
{
  NSMutableArray<MWMLaneInfo *> * result = [NSMutableArray arrayWithCapacity:lanes.size()];
  for (auto const & lane : lanes)
  {
    auto const activeWays = lane.laneWays.GetActiveLaneWays();
    NSMutableArray<NSNumber *> * laneWays = [NSMutableArray arrayWithCapacity:activeWays.size()];
    for (auto const way : activeWays)
      [laneWays addObject:@(static_cast<uint8_t>(way))];
    [result addObject:[[MWMLaneInfo alloc] initWithLaneWays:laneWays
                                             recommendedWay:static_cast<uint8_t>(lane.recommendedWay)]];
  }
  return result;
}
