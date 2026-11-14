#import "MapsAppDelegate.h"
#import "MapViewController.h"
#import "MWMViewController.h"

@implementation MWMViewController

- (BOOL)prefersStatusBarHidden
{
  return NO;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self.navigationController.navigationBar setTranslucent:NO];
}

#pragma mark - Properties

- (BOOL)hasNavigationBar
{
  return YES;
}

@end
