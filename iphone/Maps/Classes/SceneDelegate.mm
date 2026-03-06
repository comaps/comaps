#import "SceneDelegate.h"

#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMKeyboard.h"
#import "MWMLocationManager.h"
#import "MWMSearch+CoreSpotlight.h"
#import "MWMTextToSpeech.h"
#import "SwiftBridge.h"

#import <CoreApi/AppInfo.h>
#import <CoreApi/Framework.h>
#import <CoreSpotlight/CoreSpotlight.h>

#include "map/gps_tracker.hpp"

#include "base/assert.hpp"

@implementation SceneDelegate

- (void)scene:(UIScene *)scene
  willConnectToSession:(UISceneSession *)session
               options:(UISceneConnectionOptions *)connectionOptions {
  // UIKit automatically creates the UIWindow and root view controller from Main.storyboard
  // (configured via UISceneStoryboardFile in UIApplicationSceneManifest).

  // Handle cold-start deep links delivered via URL contexts.
  NSURL * launchURL = connectionOptions.URLContexts.anyObject.URL;
  if (launchURL) {
    [DeepLinkHandler.shared applicationDidFinishLaunching:@{UIApplicationLaunchOptionsURLKey: launchURL}];
  }

  // Handle cold-start universal links delivered via user activities.
  for (NSUserActivity * activity in connectionOptions.userActivities) {
    if ([activity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb] && activity.webpageURL) {
      [DeepLinkHandler.shared applicationDidReceiveUniversalLink:activity.webpageURL];
    }
  }
}

- (void)sceneDidBecomeActive:(UIScene *)scene {
  LOG(LINFO, ("sceneDidBecomeActive - begin"));

  auto & f = GetFramework();
  f.EnterForeground();
  [self.mapViewController onGetFocus:YES];
  f.SetRenderingEnabled();
  // On some devices we have to free all belong-to-graphics memory
  // because of new OpenGL driver powered by Metal.
  if ([AppInfo sharedInfo].openGLDriver == MWMOpenGLDriverMetalPre103) {
    CGSize const objcSize = self.mapViewController.mapView.pixelSize;
    f.OnRecoverSurface(static_cast<int>(objcSize.width), static_cast<int>(objcSize.height),
                       true /* recreateContextDependentResources */);
  }
  [MWMLocationManager applicationDidBecomeActive];
  [MWMSearch addCategoriesToSpotlight];
  [MWMKeyboard applicationDidBecomeActive];
  [MWMTextToSpeech applicationDidBecomeActive];
  LOG(LINFO, ("sceneDidBecomeActive - end"));
}

- (void)sceneWillResignActive:(UIScene *)scene {
  LOG(LINFO, ("sceneWillResignActive - begin"));
  [self.mapViewController onGetFocus:NO];
  auto & f = GetFramework();
  // On some devices we have to free all belong-to-graphics memory
  // because of new OpenGL driver powered by Metal.
  if ([AppInfo sharedInfo].openGLDriver == MWMOpenGLDriverMetalPre103) {
    f.SetRenderingDisabled(true);
    f.OnDestroySurface();
  } else {
    f.SetRenderingDisabled(false);
  }
  [MWMLocationManager applicationWillResignActive];
  f.EnterBackground();
  LOG(LINFO, ("sceneWillResignActive - end"));
}

- (void)sceneWillEnterForeground:(UIScene *)scene {
  LOG(LINFO, ("sceneWillEnterForeground - begin"));
  if (!GpsTracker::Instance().IsEnabled())
    return;

  MWMViewController * topVc =
    static_cast<MWMViewController *>(self.mapViewController.navigationController.topViewController);
  if (![topVc isKindOfClass:[MWMViewController class]])
    return;

  if ([MWMSettings isTrackWarningAlertShown])
    return;

  [topVc.alertController presentTrackWarningAlertWithCancelBlock:^{
    GpsTracker::Instance().SetEnabled(false);
  }];

  [MWMSettings setTrackWarningAlertShown:YES];
  LOG(LINFO, ("sceneWillEnterForeground - end"));
}

- (void)sceneDidEnterBackground:(UIScene *)scene {
  LOG(LINFO, ("sceneDidEnterBackground - begin"));
  [DeepLinkHandler.shared reset];
  [MapsAppDelegate.theApp handleDidEnterBackground];
  LOG(LINFO, ("sceneDidEnterBackground - end"));
}

- (void)scene:(UIScene *)scene openURLContexts:(NSSet<UIOpenURLContext *> *)URLContexts {
  for (UIOpenURLContext * context in URLContexts) {
    NSLog(@"scene:openURLContexts: %@", context.URL);
    [DeepLinkHandler.shared applicationDidOpenUrl:context.URL];
  }
}

- (void)scene:(UIScene *)scene continueUserActivity:(NSUserActivity *)userActivity {
  if ([userActivity.activityType isEqualToString:CSSearchableItemActionType]) {
    NSString * searchStringKey = userActivity.userInfo[CSSearchableItemActivityIdentifier];
    NSString * searchString = L(searchStringKey);
    if (searchString)
      [MapsAppDelegate.theApp searchText:searchString];
  } else if ([userActivity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb] &&
             userActivity.webpageURL != nil) {
    LOG(LINFO, ("scene:continueUserActivity: %@", userActivity.webpageURL));
    [DeepLinkHandler.shared applicationDidReceiveUniversalLink:userActivity.webpageURL];
  }
}

- (void)windowScene:(UIWindowScene *)windowScene
  performActionForShortcutItem:(UIApplicationShortcutItem *)shortcutItem
             completionHandler:(void (^)(BOOL))completionHandler {
  [self.mapViewController performAction:shortcutItem.type];
  completionHandler(YES);
}

#pragma mark - Private

- (MapViewController *)mapViewController {
  for (id vc in [(UINavigationController *)self.window.rootViewController viewControllers]) {
    if ([vc isKindOfClass:[MapViewController class]])
      return vc;
  }
  NSAssert(false, @"Please check the logic");
  return nil;
}

@end
