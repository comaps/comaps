#import "SceneDelegate.h"

#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "SwiftBridge.h"

#import <CoreApi/Framework.h>
#import <CoreSpotlight/CoreSpotlight.h>

#include "base/assert.hpp"

@implementation SceneDelegate

- (void)scene:(UIScene *)scene
  willConnectToSession:(UISceneSession *)session
               options:(UISceneConnectionOptions *)connectionOptions {
  MapsAppDelegate.theApp.activeSceneDelegate = self;

  self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
  self.window.rootViewController = [MapsAppDelegate.theApp ensureMapNavigationController];
  [self.window makeKeyAndVisible];

  // Handle cold-start deep links and document imports delivered via URL contexts.
  for (UIOpenURLContext * context in connectionOptions.URLContexts) {
    NSURL * launchURL = context.URL;
    if (!launchURL)
      continue;
    if (launchURL.isFileURL)
      [DeepLinkHandler.shared applicationDidOpenUrl:launchURL];
    else
      [DeepLinkHandler.shared applicationDidFinishLaunching:@{UIApplicationLaunchOptionsURLKey: launchURL}];
  }

  // Handle cold-start user activities (universal links and Spotlight results).
  for (NSUserActivity * activity in connectionOptions.userActivities)
    [self handleUserActivity:activity deferUniversalLinkHandling:YES];

  if (connectionOptions.shortcutItem)
    [MapsAppDelegate.theApp.mapViewController performAction:connectionOptions.shortcutItem.type];

  [[MWMCarPlayService shared] attachMapIfNeeded];
}

- (void)sceneDidDisconnect:(UIScene *)scene {
  if (MapsAppDelegate.theApp.activeSceneDelegate == self)
    MapsAppDelegate.theApp.activeSceneDelegate = nil;
}

- (void)scene:(UIScene *)scene openURLContexts:(NSSet<UIOpenURLContext *> *)URLContexts {
  for (UIOpenURLContext * context in URLContexts) {
    NSLog(@"scene:openURLContexts: %@", context.URL);
    [DeepLinkHandler.shared applicationDidOpenUrl:context.URL];
  }
}

- (void)scene:(UIScene *)scene continueUserActivity:(NSUserActivity *)userActivity {
  [self handleUserActivity:userActivity deferUniversalLinkHandling:NO];
}

- (void)windowScene:(UIWindowScene *)windowScene
  performActionForShortcutItem:(UIApplicationShortcutItem *)shortcutItem
             completionHandler:(void (^)(BOOL))completionHandler {
  [MapsAppDelegate.theApp.mapViewController performAction:shortcutItem.type];
  completionHandler(YES);
}

#pragma mark - Private


- (void)handleUserActivity:(NSUserActivity *)userActivity
    deferUniversalLinkHandling:(BOOL)deferUniversalLinkHandling {
  if ([userActivity.activityType isEqualToString:CSSearchableItemActionType]) {
    NSString * searchStringKey = userActivity.userInfo[CSSearchableItemActivityIdentifier];
    NSString * searchString = L(searchStringKey);
    if (searchString)
      [MapsAppDelegate.theApp searchText:searchString];
  } else if ([userActivity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb] &&
             userActivity.webpageURL != nil) {
    LOG(LINFO, ("scene continueUserActivity: %@", userActivity.webpageURL));
    if (deferUniversalLinkHandling)
      [DeepLinkHandler.shared applicationDidFinishLaunchingWithUniversalLink:userActivity.webpageURL];
    else
      [DeepLinkHandler.shared applicationDidReceiveUniversalLink:userActivity.webpageURL];
  }
}

@end
