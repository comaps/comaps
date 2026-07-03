#import "MWMSearchCell.h"
#import "SearchResult.h"

@interface MWMSearchCell ()

@property(weak, nonatomic) IBOutlet UILabel *titleLabel;

@end

@implementation MWMSearchCell

- (void)configureWith:(SearchResult *_Nonnull)result isPartialMatching:(BOOL)isPartialMatching
{
  NSString *title = result.titleText;

  if (title.length == 0)
  {
    self.titleLabel.text = @"";
    return;
  }

  bool hasBranchName = (result.branchText && result.branchText.length > 0 && ![title containsString:result.branchText]);

  NSDictionary *selectedTitleAttributes = [self selectedTitleAttributes];
  NSDictionary *unselectedTitleAttributes = [self unselectedTitleAttributes];
  if ((!selectedTitleAttributes || !unselectedTitleAttributes) && !hasBranchName)
  {
    self.titleLabel.text = title;
    return;
  }

  NSMutableAttributedString *attributedTitle = [[NSMutableAttributedString alloc] initWithString:title];

  NSArray<NSValue *> *highlightRanges = result.highlightRanges;
  [attributedTitle addAttributes:unselectedTitleAttributes range:NSMakeRange(0, title.length)];

  // Add branch with thinner font weight if present and not already in title
  if (hasBranchName)
  {
    NSMutableDictionary *branchAttributes = [unselectedTitleAttributes mutableCopy];
    [branchAttributes setValue:[UIFont regular17] forKey:NSFontAttributeName];
    NSAttributedString *branchString =
        [[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@" %@", result.branchText]
                                        attributes:branchAttributes];
    [attributedTitle appendAttributedString:branchString];
  }

  for (NSValue *rangeValue in highlightRanges)
  {
    NSRange range = [rangeValue rangeValue];
    if (NSMaxRange(range) <= attributedTitle.string.length)
      [attributedTitle setAttributes:selectedTitleAttributes range:range];
    else
      NSLog(@"Incorrect range: %@ for string: %@", NSStringFromRange(range), attributedTitle.string);
  }

  self.titleLabel.attributedText = attributedTitle;
  [self.titleLabel sizeToFit];
}

- (NSDictionary *)selectedTitleAttributes
{
  return nil;
}

- (NSDictionary *)unselectedTitleAttributes
{
  return nil;
}

@end
