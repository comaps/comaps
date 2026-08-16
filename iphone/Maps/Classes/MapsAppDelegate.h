#import "DownloadIndicatorProtocol.h"
#import "MWMNavigationController.h"

@class MapViewController;
@class MWMCarPlayService;
@class SceneDelegate;

NS_ASSUME_NONNULL_BEGIN

@interface MapsAppDelegate : UIResponder<UIApplicationDelegate, DownloadIndicatorProtocol>
{
  NSInteger m_activeDownloadsCounter;
  UIBackgroundTaskIdentifier m_backgroundTask;
}

// Nil if launched directly into CarPlay
@property(nonatomic, readonly, nullable) UIWindow * window;
@property(nonatomic, weak, nullable) SceneDelegate * activeSceneDelegate;
@property(nonatomic, readonly) MWMCarPlayService *carplayService API_AVAILABLE(ios(12.0));
@property(nonatomic, readonly, nullable) MWMNavigationController * mapNavigationController;
@property(nonatomic, readonly, nullable) MapViewController * mapViewController;
@property(nonatomic, readonly) BOOL isDrapeEngineCreated;

+ (MapsAppDelegate *)theApp;

- (MWMNavigationController *)ensureMapNavigationController;

- (void)enableStandby;
- (void)disableStandby;

+ (void)customizeAppearance;
+ (void)customizeAppearanceForNavigationBar:(UINavigationBar *)navigationBar;

- (void)disableDownloadIndicator;
- (void)enableDownloadIndicator;

- (void)searchText:(NSString *)searchString;
- (void)showMap;

- (NSUInteger)badgeNumber;

+ (BOOL)isTestsEnvironment;

@end

NS_ASSUME_NONNULL_END
