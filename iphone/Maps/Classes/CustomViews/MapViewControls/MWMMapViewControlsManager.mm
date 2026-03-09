#import "MWMMapViewControlsManager.h"
#import "MWMAddPlaceNavigationBar.h"
#import "MWMMapDownloadDialog.h"
#import "MWMMapViewControlsManager+AddPlace.h"
#import "MWMNetworkPolicy+UI.h"
#import "MWMPlacePageManager.h"
#import "MWMPlacePageProtocol.h"
#import "MWMMapWidgetsHelper.h"
#import "MapViewController.h"
#import "MapsAppDelegate.h"
#import "SwiftBridge.h"

#include <CoreApi/Framework.h>
#import <CoreApi/MWMFrameworkHelper.h>

#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "storage/storage_helpers.hpp"

#include "map/place_page_info.hpp"

namespace {
NSString *const kMapToCategorySelectorSegue = @"MapToCategorySelectorSegue";
}  // namespace

@interface MWMMapViewControlsManager ()

@property(nonatomic) UIButton * promoButton;
@property(nonatomic) UIViewController * menuController;
@property(nonatomic) id<MWMPlacePageProtocol> placePageManager;
@property(nonatomic) MWMNavigationDashboardManager * navigationManager;
@property(nonatomic) SearchOnMapManager * searchManager;

@property(weak, nonatomic) MapViewController * ownerController;

@property(nonatomic) BOOL disableStandbyOnRouteFollowing;
@property(nonatomic) BOOL isAddingPlace;

@end

@implementation MWMMapViewControlsManager

+ (MWMMapViewControlsManager *)manager {
  return [MapViewController sharedController].controlsManager;
}

- (instancetype)initWithParentController:(MapViewController *)controller {
  if (!controller)
    return nil;
  self = [super init];
  if (!self)
    return nil;
  self.ownerController = controller;
  self.hidden = NO;
  self.isDirectionViewHidden = YES;
  self.isAddingPlace = NO;
  self.searchManager = controller.searchManager;
  return self;
}

- (UIStatusBarStyle)preferredStatusBarStyle {
  BOOL const isNavigationUnderStatusBar = self.navigationManager.state != MWMNavigationDashboardStateHidden &&
                                          self.navigationManager.state != MWMNavigationDashboardStateNavigation;
  BOOL const isDirectionViewUnderStatusBar = !self.isDirectionViewHidden;
  BOOL const isAddPlaceUnderStatusBar =
    [self.ownerController.view hasSubviewWithViewClass:[MWMAddPlaceNavigationBar class]];
  BOOL const isNightMode = [UIColor isNightMode];
  BOOL const isSomethingUnderStatusBar = isNavigationUnderStatusBar ||
                                         isDirectionViewUnderStatusBar ||
                                         isAddPlaceUnderStatusBar;

  return isSomethingUnderStatusBar || isNightMode ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;
}

#pragma mark - MWMPlacePageViewManager

- (void)searchOnMap:(SearchQuery *)query {
  if (![self search:query])
    return;

  [self.searchManager startSearchingWithIsRouting:NO];
}

- (BOOL)search:(SearchQuery *)query {
  if (query.text.length == 0)
    return NO;

  [self.searchManager startSearchingWithIsRouting:NO];
  [self.searchManager searchText:query];
  return YES;
}

#pragma mark - BottomMenu
- (void)actionDownloadMaps:(MWMMapDownloaderMode)mode {
  [self.ownerController openMapsDownloader:mode];
}

- (void)didFinishAddingPlace {
  self.isAddingPlace = NO;

  if (![MWMRouter isRoutingActive])
    MapControls.areMapControlsHidden = false;
  MapControls.areMapZoomButtonsHidden = false;
}

- (void)addPlace {
  [self addPlace:NO position:nullptr];
}

- (void)addPlace:(BOOL)isBusiness position:(m2::PointD const *)optionalPosition {
  MapViewController *ownerController = self.ownerController;

  self.isAddingPlace = YES;
  [self.searchManager close];

  MapControls.areMapControlsHidden = true;
  MapControls.areMapZoomButtonsHidden = true;

  [ownerController dismissPlacePage];

  [MWMAddPlaceNavigationBar showInSuperview:ownerController.view
    isBusiness:isBusiness
    position:optionalPosition
    doneBlock:^{
      if ([MWMFrameworkHelper canEditMapAtViewportCenter])
        [ownerController performSegueWithIdentifier:kMapToCategorySelectorSegue sender:nil];
      else
        [ownerController.alertController presentIncorrectFeauturePositionAlert];

      [self didFinishAddingPlace];
    }
    cancelBlock:^{
      [self didFinishAddingPlace];
    }];
  [ownerController setNeedsStatusBarAppearanceUpdate];
}

#pragma mark - MWMNavigationDashboardManager

- (void)setDisableStandbyOnRouteFollowing:(BOOL)disableStandbyOnRouteFollowing {
  if (_disableStandbyOnRouteFollowing == disableStandbyOnRouteFollowing)
    return;
  _disableStandbyOnRouteFollowing = disableStandbyOnRouteFollowing;
  if (disableStandbyOnRouteFollowing)
    [[MapsAppDelegate theApp] disableStandby];
  else
    [[MapsAppDelegate theApp] enableStandby];
}

#pragma mark - Routing

- (void)onRoutePrepare {
  auto nm = self.navigationManager;
  [nm onRoutePrepare];
  [nm onRoutePointsUpdated];
  [self.ownerController.bookmarksCoordinator close];
  self.promoButton.hidden = YES;
}

- (void)onRouteRebuild {
  [self.ownerController.bookmarksCoordinator close];
  [self.navigationManager onRoutePlanning];
  self.promoButton.hidden = YES;
}

- (void)onRouteReady:(BOOL)hasWarnings {
  [self.navigationManager onRouteReady:hasWarnings];
  self.promoButton.hidden = YES;
}

- (void)onRouteStart {
  self.hidden = NO;
  MapControls.areMapControlsHidden = true;
  self.disableStandbyOnRouteFollowing = YES;
  [self.navigationManager onRouteStart];
  self.promoButton.hidden = YES;
}

- (void)onRouteStop {
  [self.navigationManager onRouteStop];
  self.disableStandbyOnRouteFollowing = NO;
  self.promoButton.hidden = YES;
}

#pragma mark - Properties

- (id<MWMPlacePageProtocol>)placePageManager {
  if (!_placePageManager)
    _placePageManager = [[MWMPlacePageManager alloc] init];
  return _placePageManager;
}

- (MWMNavigationDashboardManager *)navigationManager {
  if (!_navigationManager)
    _navigationManager = [[MWMNavigationDashboardManager alloc] initWithParentView:self.ownerController.mainView];
  return _navigationManager;
}

- (void)setHidden:(BOOL)hidden {
  MapControls.areMapControlsHidden = hidden;
  MapControls.areMapZoomButtonsHidden = hidden;

  if (_hidden == hidden)
    return;
  // Do not hide the controls view during the place adding process.
  if (!_isAddingPlace)
    _hidden = hidden;
}

#pragma mark - MWMFeatureHolder

- (id<MWMFeatureHolder>)featureHolder {
  return self.placePageManager;
}

@end
