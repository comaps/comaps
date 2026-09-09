#import "AccessibilityBridging.h"

#include <CoreApi/Framework.h>

#include <type_traits>

static dp::TAccessibilityStableIDContainer allNodes;

static_assert(std::is_same<dp::TAccessibilityStableID, int>::value);
// ensure a match with the typedef in AccessibilityBriding.h (can't do it there due to PIMPL)

@implementation AccessibilityBridging

+ (dp::TAccessibilityStableID)getAccessibilityNodeAtPoint:(CGPoint)point
{
  ref_ptr<df::DrapeEngine> engine = GetFramework().GetDrapeEngine();
  if (engine)
  {
    auto presenter = engine->GetAccessibilityPresenter();
    if (presenter)
      return (*presenter)->GetNodeAtPoint(m2::PointD(point.x, point.y));
  }
  return 0;
}

+ (NSArray<NSNumber *> *)getAllAccessibilityNodes
{
  ref_ptr<df::DrapeEngine> engine = GetFramework().GetDrapeEngine();
  if (engine)
  {
    auto presenter = engine->GetAccessibilityPresenter();
    if (presenter)
    {
      (*presenter)->GetAllNodes(allNodes);
      NSMutableArray<NSNumber *> * ret = [NSMutableArray arrayWithCapacity:allNodes.size()];
      for (auto const & node : allNodes)
      {
        static_assert(std::is_same<int, dp::TAccessibilityStableID>::value);
        [ret addObject:[NSNumber numberWithInt:node]];
      }
      return ret;
    }
  }

  return @[];
}

+ (dp::AccessibilityNodeContext *)getAccessibilityNodeContext:(dp::TAccessibilityStableID)id
{
  ref_ptr<df::DrapeEngine> engine = GetFramework().GetDrapeEngine();
  if (engine)
  {
    auto presenter = engine->GetAccessibilityPresenter();
    if (presenter)
    {
      auto overlay = (*presenter)->GetNode(id);
      if (overlay)
        return overlay->get();
    }
  }
  return 0;
}

+ (BOOL)setAccessibilityUpdateCallback:(void (^)(NSArray<NSNumber *> *, NSArray<NSNumber *> *, NSArray<NSNumber *> *))cb
{
  ref_ptr<df::DrapeEngine> engine = GetFramework().GetDrapeEngine();
  if (engine)
  {
    auto presenter = engine->GetAccessibilityPresenter();

    if (static_cast<bool>(presenter) != static_cast<bool>(cb))
    {
      std::optional<drape_ptr<dp::AccessibilityPresenter>> new_presenter;
      if (cb)
	new_presenter = make_unique_dp<dp::AccessibilityPresenter>();
      else
	new_presenter = {};
      engine->SetAccessibilityPresenter(std::move(new_presenter));
      presenter = engine->GetAccessibilityPresenter();
    }

    if (presenter)
    {
      (*presenter)->SetUpdateCallback([cb](std::vector<dp::TAccessibilityStableID> removed, std::vector<dp::TAccessibilityStableID> updated, std::vector<dp::TAccessibilityStableID> added){
        static_assert(std::is_same<int, dp::TAccessibilityStableID>::value);

        const auto removedNS = [NSMutableArray arrayWithCapacity:removed.size()];
        const auto updatedNS = [NSMutableArray arrayWithCapacity:updated.size()];
        const auto addedNS = [NSMutableArray arrayWithCapacity:added.size()];

        for (auto const & node : removed)
        {
            static_assert(std::is_same<int, dp::TAccessibilityStableID>::value);
          [removedNS addObject:[NSNumber numberWithInt:node]];
        }
        for (auto const & node : updated)
        {
          static_assert(std::is_same<int, dp::TAccessibilityStableID>::value);
          [updatedNS addObject:[NSNumber numberWithInt:node]];
        }
        for (auto const & node : added)
        {
          static_assert(std::is_same<int, dp::TAccessibilityStableID>::value);
          [addedNS addObject:[NSNumber numberWithInt:node]];
        }

        cb(removedNS, updatedNS, addedNS);
      });
      return true;
    }
  }
  return !cb;
}

+ (void)setAccessibilityFreezeFrame:(BOOL)freeze
{
  // we could just freeze the accessibility presenter, but better to freeze the whole drape engine so that the screen aligns
  // (e.g. for low vision users who still use sight during touch exploration)
  if (freeze)
    GetFramework().SetRenderingDisabled(false /* destroySurface */);
  else
    GetFramework().SetRenderingEnabled();

}

@end
