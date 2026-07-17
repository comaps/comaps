#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Extracts data from the C++ core for the watch companion app: simplified
/// vector geometry around the current route ("corridor") and the route's
/// elevation profile. Call on the main thread; the corridor extraction hops
/// to a background queue internally because it reads map features from disk.
@interface MWMWatchMapExtractor : NSObject

/// Packed corridor geometry (roads, water, rail) around the current route,
/// delivered on the main queue; nil when no valid route is built.
/// See WatchMapContext.swift for the binary layout.
+ (void)extractCorridorWithTimestamp:(NSTimeInterval)timestamp
                          completion:(void (^)(NSData *_Nullable data))completion;

/// Distance/altitude pairs (packed Float32 pairs, meters) along the current
/// route downsampled to at most maxPoints, or nil when the route has no
/// altitude information. Total ascent/descent are computed from the full
/// resolution data.
+ (nullable NSData *)routeAltitudesWithMaxPoints:(NSUInteger)maxPoints
                                          ascent:(double *)totalAscent
                                         descent:(double *)totalDescent;

/// Bookmarks as watch destinations: dictionaries with the keys declared in
/// WatchDestination.swift ("n" name, "la" latitude, "lo" longitude).
+ (NSArray<NSDictionary<NSString *, id> *> *)watchDestinationsWithLimit:(NSUInteger)limit;

/// Builds a route from the current location to the given place, replacing any
/// active route. The result reaches the watch through the normal route sync.
+ (void)buildRouteToLatitude:(double)latitude longitude:(double)longitude name:(NSString *)name;

/// Saves a bookmark (into the last edited bookmark list), so a spot marked on
/// the watch becomes a destination like any other bookmark.
+ (void)addBookmarkAtLatitude:(double)latitude longitude:(double)longitude name:(NSString *)name;

/// The next turn of the active navigation session as a WatchTurnInfo
/// dictionary (SF Symbol name + localized distance + next street), or nil
/// when not navigating. Call on the main thread.
+ (nullable NSDictionary<NSString *, id> *)currentTurnInfo;

@end

NS_ASSUME_NONNULL_END
