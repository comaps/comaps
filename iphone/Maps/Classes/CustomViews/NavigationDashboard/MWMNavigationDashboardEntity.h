@class MWMRouterTransitStepInfo;
@class MWMLaneInfo;
@class MWMRoadShieldInfo;

@interface MWMNavigationDashboardEntity : NSObject

@property(copy, nonatomic, readonly) NSArray<MWMRouterTransitStepInfo *> *transitSteps;
@property(copy, nonatomic, readonly) NSArray<MWMLaneInfo *> *lanes;
@property(copy, nonatomic, readonly) NSString *distanceToTurn;
@property(copy, nonatomic, readonly) NSString *streetName;

// Structured (shield-resolved) components of the next turn's road, used to compose a turn
// instruction with inline drawn road shields. See -attributedInstructionForTextSize:textColor:.
@property(copy, nonatomic, readonly) NSString *nextRoadName;
@property(copy, nonatomic, readonly) NSString *nextRoadRef;
@property(copy, nonatomic, readonly) NSString *nextJunctionRef;
@property(copy, nonatomic, readonly) NSString *nextDestinationRef;
@property(copy, nonatomic, readonly) NSString *nextDestination;
@property(nonatomic, readonly) BOOL nextIsLink;
@property(nonatomic, readonly) BOOL isLeftHandTraffic;
@property(nonatomic, readonly) MWMRoadShieldInfo *nextRoadShields;

@property(copy, nonatomic, readonly) NSString *targetDistance;
@property(copy, nonatomic, readonly) NSString *targetUnits;
@property(copy, nonatomic, readonly) NSString *turnUnits;
@property(nonatomic, readonly) double speedLimitMps;
@property(nonatomic, readonly) BOOL isValid;
@property(nonatomic, readonly) CGFloat progress;
@property(nonatomic, readonly) NSUInteger roundExitNumber;
@property(nonatomic, readonly) NSUInteger timeToTarget;
@property(nonatomic, readonly) UIImage *nextTurnImage;
@property(nonatomic, readonly) UIImage *turnImage;

@property(nonatomic, readonly) NSString * arrival;

// Composes the turn instruction with inline drawn road shields sized for the given label styling.
// Returns nil when there is nothing to compose (caller should fall back to -streetName).
- (NSAttributedString *)attributedInstructionForTextSize:(CGFloat)textSize textColor:(UIColor *)textColor;

- (NSAttributedString *) estimate;

+ (NSAttributedString *) estimateDot;

+ (instancetype) new __attribute__((unavailable("init is not available")));

@end
