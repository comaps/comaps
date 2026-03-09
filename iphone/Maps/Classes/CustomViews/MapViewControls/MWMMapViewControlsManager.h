#import "MWMMapDownloaderMode.h"
#import "MWMNavigationDashboardManager.h"

@class MapViewController;
@class SearchQuery;

@protocol MWMFeatureHolder;

@interface MWMMapViewControlsManager : NSObject

+ (MWMMapViewControlsManager *)manager NS_SWIFT_NAME(manager());

@property(nonatomic) BOOL hidden;
@property(nonatomic) BOOL isDirectionViewHidden;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentController:(MapViewController *)controller;

- (UIStatusBarStyle)preferredStatusBarStyle;

- (void)addPlace;

#pragma mark - MWMNavigationDashboardManager

- (void)onRoutePrepare;
- (void)onRouteRebuild;
- (void)onRouteReady:(BOOL)hasWarnings;
- (void)onRouteStart;
- (void)onRouteStop;

#pragma mark - MWMSearchManager

- (void)actionDownloadMaps:(MWMMapDownloaderMode)mode;
- (BOOL)search:(SearchQuery *)query;
- (void)searchOnMap:(SearchQuery *)query;

#pragma mark - MWMFeatureHolder

- (id<MWMFeatureHolder>)featureHolder;

@end
