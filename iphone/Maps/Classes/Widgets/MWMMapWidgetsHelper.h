@interface MWMMapWidgetsHelper : NSObject

+ (void)updateLayout;
+ (void)updatePaddingForTop:(CGFloat)top
                     bottom:(CGFloat)bottom
                    leading:(CGFloat)leading
              bottomLeading:(CGFloat)bottomLeading
                   trailing:(CGFloat)trailing;

@end
