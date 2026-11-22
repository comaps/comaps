#import "MWMMapWidgetsHelper.h"
#import "MWMMapWidgets.h"

@implementation MWMMapWidgetsHelper

+ (void)updateLayout
{
  [[MWMMapWidgets widgetsManager] updateLayout];
}

+ (void)updatePaddingForTop:(CGFloat)top
                     bottom:(CGFloat)bottom
                    leading:(CGFloat)leading
              bottomLeading:(CGFloat)bottomLeading
                   trailing:(CGFloat)trailing
{
  [[MWMMapWidgets widgetsManager] updatePaddingForTop:top
                                               bottom:bottom
                                              leading:leading
                                        bottomLeading:bottomLeading
                                             trailing:trailing];
}

@end
