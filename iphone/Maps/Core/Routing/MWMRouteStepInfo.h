@interface MWMRouteStepInfo : NSObject

@property(nonatomic, readonly) uint32_t index;
@property(nonatomic, readonly) NSString * _Nullable fromStreetName;
@property(nonatomic, readonly) NSString * _Nullable toStreetName;
@property(nonatomic, readonly) uint32_t exitNum;
@property(nonatomic, readonly) double distMeters;
@property(nonatomic, readonly) NSString * _Nonnull formattedDistance;
@property(nonatomic, readonly) NSString * _Nonnull textualInstruction;
@property(nonatomic, readonly) int32_t carDirection;
@property(nonatomic, readonly) int32_t pedestrianDirection;

@end
