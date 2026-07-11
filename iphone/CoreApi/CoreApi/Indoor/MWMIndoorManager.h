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
+ (NSArray<NSString *> *)levels;
+ (NSString *)activeLevel;
+ (void)selectLevel:(NSString *)level;

@end

NS_ASSUME_NONNULL_END
