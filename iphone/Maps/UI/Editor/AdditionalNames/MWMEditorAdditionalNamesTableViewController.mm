#import "MWMEditorAdditionalNamesTableViewController.h"
#import "MWMTableViewCell.h"

#import <CoreApi/StringUtils.h>

@interface MWMEditorAdditionalNamesTableViewController ()

@property (weak, nonatomic) id<MWMEditorAdditionalNamesProtocol> delegate;

@end

@implementation MWMEditorAdditionalNamesTableViewController
{
  StringUtf8Multilang m_name;
  std::vector<localisation::Language> m_languages;
  std::vector<NSInteger> m_additionalSkipLanguageCodes;
}

#pragma mark - UITableViewDataSource

- (void)configWithDelegate:(id<MWMEditorAdditionalNamesProtocol>)delegate
                      name:(StringUtf8Multilang const &)name
additionalSkipLanguageCodes:(std::vector<NSInteger>)additionalSkipLanguageCodes
{
  self.delegate = delegate;
  m_name = name;
  m_additionalSkipLanguageCodes = additionalSkipLanguageCodes;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.title = L(@"choose_language");
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  auto const getLanguageIndex = [](std::string languageCode) { return localisation::ConvertLanguageCodeToLanguageIndex(languageCode); };
  m_languages.clear();

  for (auto const & language : localisation::GetSupportedLanguages())
  {
    auto const languageIndex = getLanguageIndex(language.m_languageCode);
    if (languageIndex != localisation::kDefaultNameIndex && m_name.HasString(languageIndex))
      continue;
    auto it = std::find(m_additionalSkipLanguageCodes.begin(), m_additionalSkipLanguageCodes.end(), languageIndex);
    if (it == m_additionalSkipLanguageCodes.end())
      m_languages.push_back(language);
  }

  std::sort(m_languages.begin(), m_languages.end(),
       [&getLanguageIndex](localisation::Language const & lhs, localisation::Language const & rhs) {
         // Default name can be changed in advanced mode, but it should be last in list of names.
         if (getLanguageIndex(lhs.m_languageCode) == localisation::kDefaultNameIndex && getLanguageIndex(rhs.m_languageCode) != localisation::kDefaultNameIndex)
           return false;
         if (getLanguageIndex(lhs.m_languageCode) != localisation::kDefaultNameIndex && getLanguageIndex(rhs.m_languageCode) == localisation::kDefaultNameIndex)
           return true;

         return std::string(lhs.m_languageCode) < std::string(rhs.m_languageCode);
       });
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  MWMTableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"ListCellIdentifier"];
  NSInteger const index = indexPath.row;
  localisation::Language const & lang = m_languages[index];
  cell.textLabel.text = ToNSString(lang.m_name);
  cell.detailTextLabel.text = ToNSString(lang.m_languageCode);
  cell.accessoryType = UITableViewCellAccessoryNone;
  return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_languages.size();
}

#pragma mark - UITableViewDataSource

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSInteger const index = indexPath.row;
  localisation::Language const & language = m_languages[index];

  [self.delegate addAdditionalName:localisation::ConvertLanguageCodeToLanguageIndex(language.m_languageCode)];
  [self.navigationController popViewControllerAnimated:YES];
}

@end
