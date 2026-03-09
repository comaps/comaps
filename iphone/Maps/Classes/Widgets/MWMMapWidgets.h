#include <CoreApi/Framework.h>

@interface MWMMapWidgets : NSObject

+ (MWMMapWidgets *)widgetsManager;

- (void)setupWidgets:(Framework::DrapeCreationParams &)p;

- (void)resize:(CGSize)size;
- (void)updateLayout;
- (void)updatePaddingForTop:(CGFloat)top bottom:(CGFloat)bottom leading:(CGFloat)leading trailing:(CGFloat)trailing;

@end
