#import "UIButton+Orientation.h"
#import "UIViewController+Navigation.h"

static CGFloat const kButtonExtraWidth = 16.0;

@implementation UIViewController (Navigation)

- (UIBarButtonItem *)negativeSpacer
{
  UIBarButtonItem *spacer = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFixedSpace
                                                                          target:nil
                                                                          action:nil];
  spacer.width = -kButtonExtraWidth;
  return spacer;
}

- (UIBarButtonItem *)buttonWithImage:(UIImage *)image action:(SEL)action
{
  return [[UIBarButtonItem alloc] initWithImage:image style:UIBarButtonItemStylePlain target:self action:action];
}

- (NSArray<UIBarButtonItem *> *)alignedNavBarButtonItems:(NSArray<UIBarButtonItem *> *)items
{
  return [@[[self negativeSpacer]] arrayByAddingObjectsFromArray:items];
}

- (void)goBack
{
  [self.navigationController popViewControllerAnimated:YES];
}

@end
