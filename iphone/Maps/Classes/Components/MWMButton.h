typedef NS_ENUM(NSUInteger, MWMButtonColoring)
{
  MWMButtonColoringOther,
  MWMButtonColoringBlue,
  MWMButtonColoringBlack,
  MWMButtonColoringWhite,
  MWMButtonColoringWhiteText,
  MWMButtonColoringGray,
  MWMButtonColoringRed
};

@interface MWMButton : UIButton

@property (copy, nonatomic) NSString * imageName;
@property (copy, nonatomic) NSString * backgroundImageName;
@property (nonatomic) MWMButtonColoring coloring;

@end
