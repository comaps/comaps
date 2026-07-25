#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol MWMIndoorObserver <NSObject>

/// Called on the main thread when the set of indoor levels in the viewport changes.
- (void)onIndoorLevelsUpdated;

@end

NS_SWIFT_NAME(IndoorManager)
@interface MWMIndoorManager : NSObject

+ (void)addObserver:(id<MWMIndoorObserver>)observer;
+ (void)removeObserver:(id<MWMIndoorObserver>)observer;

/// Level labels sorted from the topmost floor down; empty when no indoor data is visible.
/// Cached from the last observer notification.
+ (NSArray<NSString *> *)levels;

/// Current viewport levels read straight from the core (topmost floor first), independent of the
/// observer cache. Lets a freshly created UI re-sync immediately without waiting for the next
/// levels-changed notification. Mirrors Android's IndoorManager.getViewportLevels().
+ (NSArray<NSString *> *)viewportLevels;

+ (NSString *)activeLevel;
+ (void)selectLevel:(NSString *)level;

@end

NS_ASSUME_NONNULL_END
