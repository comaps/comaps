#import <Foundation/Foundation.h>

#ifdef __cplusplus
#include <drape/accessibility_node_context.hpp>
#include <drape/accessibility_data.hpp>
typedef dp::TAccessibilityStableID TAccessibilityStableID;
typedef dp::AccessibilityNodeContext AccessibilityNodeContext;
typedef dp::AccessibilityNodeInfo AccessibilityNodeInfo;
typedef m2::RectD RectD;
#else
// PIMPL
typedef int TAccessibilityStableID;
typedef struct{} AccessibilityNodeContext;
typedef struct{} AccessibilityNodeInfo;
typedef struct{} RectD;
#endif

NS_SWIFT_NAME(AccessibilityBridging)
@interface AccessibilityBridging : NSObject

+ (TAccessibilityStableID)getAccessibilityNodeAtPoint:(CGPoint)point;
+ (NSArray<NSNumber *> *_Nonnull)getAllAccessibilityNodes;
+ (AccessibilityNodeContext *_Nullable)getAccessibilityNodeContext:(TAccessibilityStableID)id;
+ (BOOL)setAccessibilityUpdateCallback:(void (^)(NSArray<NSNumber *> *_Nonnull, NSArray<NSNumber *> *_Nonnull, NSArray<NSNumber *> *_Nonnull))callback;
+ (void)setAccessibilityFreezeFrame:(BOOL)freeze;

@end
