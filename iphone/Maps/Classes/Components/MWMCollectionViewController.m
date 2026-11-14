#import "MWMCollectionViewController.h"
#import "MapViewController.h"
#import "SwiftBridge.h"

@implementation MWMCollectionViewController

- (BOOL)prefersStatusBarHidden { return NO; }

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.collectionView.styleName = @"PressBackground";
  [self.navigationController.navigationBar setTranslucent:NO];
}

#pragma mark - Properties

- (BOOL)hasNavigationBar { return YES; }

@end
