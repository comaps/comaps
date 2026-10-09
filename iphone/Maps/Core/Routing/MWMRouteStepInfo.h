@interface MWMRouteStepInfo : NSObject

@property(nonatomic, readonly) uint32_t index;
@property(nonatomic, readonly) uint32_t turnIndex;
@property(nonatomic, readonly) NSString * _Nullable fromStreetName;
@property(nonatomic, readonly) NSString * _Nullable toStreetName;
@property(nonatomic, readonly) NSString * _Nonnull toRef;
@property(nonatomic, readonly) NSString * _Nonnull toJunctionRef;
@property(nonatomic, readonly) NSString * _Nonnull toDestinationRef;
@property(nonatomic, readonly) NSString * _Nonnull toDestination;
@property(nonatomic, readonly) BOOL toIsLink;
@property(nonatomic, readonly) NSArray * _Nonnull lanes;
@property(nonatomic, readonly) uint32_t exitNum;
@property(nonatomic, readonly) double distMeters;
@property(nonatomic, readonly) NSString * _Nonnull formattedDistance;
@property(nonatomic, readonly) NSString * _Nonnull textualInstruction;
@property(nonatomic, readonly) int32_t carDirection;
@property(nonatomic, readonly) int32_t pedestrianDirection;

@end
