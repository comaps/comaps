#include "base/localisation.hpp"

#include "coding/transliteration.hpp"

#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

namespace localisation
{
using namespace std;

optional<LanguageCode> GetCustomMapLanguageCode()
{
  LanguageCode mapLanguageCode;
  UNUSED_VALUE(settings::Get(kMapLanguageSetting, mapLanguageCode));
  if (!mapLanguageCode.empty())
    return mapLanguageCode;

  return {};
}

AlternativeMapLanguageHandling UsedAlternativeMapLanguageHandling()
{
  AlternativeMapLanguageHandling alternativeMapLanguageHandling = AlternativeMapLanguageHandling::LocalOnly;
  UNUSED_VALUE(settings::Get(kAlternativeMapLanguageHandlingSetting, alternativeMapLanguageHandling));
  return alternativeMapLanguageHandling;
}

bool ShouldUseTransliteration()
{
  Transliteration::Mode transliterationMode = Transliteration::Mode::Disabled;
  UNUSED_VALUE(settings::Get(kTransliterationSetting, transliterationMode));
  return transliterationMode == Transliteration::Mode::Enabled;
}

vector<LanguageCode> GetSystemLanguageCodes()
{
  return languages::GetSystemPreferredLanguageCodes();
}

LanguageCode GetInterfaceLanguageCode()
{
  return GetSystemLanguageCodes().front();
}

vector<LanguageCode> GetSimplifiedSystemLanguageCodes()
{
  vector<LanguageCode> simplifiedLanguageCodes = {};
  for (auto const & languageCode : GetSystemLanguageCodes())
  {
    LanguageCode const simplifiedLanguageCode = string{languageCode.substr(0, languageCode.find_first_of("-_ "))};
    if (find(simplifiedLanguageCodes.begin(), simplifiedLanguageCodes.end(), simplifiedLanguageCode) ==
        simplifiedLanguageCodes.end())
      simplifiedLanguageCodes.push_back(simplifiedLanguageCode);
  }
  return simplifiedLanguageCodes;
}

vector<LanguageIndex> GetSimplifiedSystemLanguageIndexes()
{
  return ConvertLanguageCodesToLanguageIndexes(GetSimplifiedSystemLanguageCodes());
}

vector<LanguageCode> GetMapLanguageCodes()
{
  vector<LanguageCode> languageCodes = GetSimplifiedSystemLanguageCodes();
  optional<LanguageCode> const customMapLanguageCode = GetCustomMapLanguageCode();
  if (customMapLanguageCode.has_value())
    languageCodes.insert(languageCodes.begin(), customMapLanguageCode.value());
  if (UsedAlternativeMapLanguageHandling() == AlternativeMapLanguageHandling::IgnoreAlternatives)
  {
    vector<LanguageCode> noAlternativeLanguageCodes;
    noAlternativeLanguageCodes.push_back(languageCodes.front());
    return noAlternativeLanguageCodes;
  }
  return languageCodes;
}

LanguageCode GetMapLanguageCode()
{
  return GetMapLanguageCodes().front();
}

vector<LanguageIndex> GetMapLanguageIndexes()
{
  vector<LanguageIndex> languageIndexes = GetSimplifiedSystemLanguageIndexes();
  optional<LanguageCode> const customMapLanguageCode = GetCustomMapLanguageCode();
  if (customMapLanguageCode.has_value())
    languageIndexes.insert(languageIndexes.begin(), ConvertLanguageCodeToLanguageIndex(customMapLanguageCode.value()));
  if (UsedAlternativeMapLanguageHandling() == AlternativeMapLanguageHandling::IgnoreAlternatives)
  {
    vector<LanguageIndex> noAlternativeLanguageIndexes;
    noAlternativeLanguageIndexes.push_back(languageIndexes.front());
    return noAlternativeLanguageIndexes;
  }
  return languageIndexes;
}

LanguageIndex GetMapLanguageIndex()
{
  return GetMapLanguageIndexes().front();
}

vector<LanguageIndex> PrioritizedMapLanguageIndexes(vector<LanguageIndex> const regionalLanguageIndexes)
{
  AlternativeMapLanguageHandling const alternativeMapLanguageHandling = UsedAlternativeMapLanguageHandling();

  vector<LanguageIndex> prioritizedMapLanguageIndexes = {};
  vector<LanguageIndex> const mapLanguageIndexes = GetMapLanguageIndexes();

  LanguageIndex defaultLang = kUnsupportedLanguageIndex;
  if (!regionalLanguageIndexes.empty())
    defaultLang = MatchingRegionalLanguageIndex(regionalLanguageIndexes, mapLanguageIndexes);

  for (LanguageIndex const mapLanguageIndex : mapLanguageIndexes)
  {
    if (alternativeMapLanguageHandling != AlternativeMapLanguageHandling::IgnoreAlternatives ||
        prioritizedMapLanguageIndexes.empty())
    {
      if (alternativeMapLanguageHandling != AlternativeMapLanguageHandling::LocalOnly &&
          find(prioritizedMapLanguageIndexes.begin(), prioritizedMapLanguageIndexes.end(), mapLanguageIndex) ==
              prioritizedMapLanguageIndexes.end())
        prioritizedMapLanguageIndexes.push_back(mapLanguageIndex);

      if (defaultLang != kUnsupportedLanguageIndex && defaultLang == mapLanguageIndex)
      {
        prioritizedMapLanguageIndexes.push_back(defaultLang);
        prioritizedMapLanguageIndexes.push_back(kDefaultNameIndex);
      }

      if (alternativeMapLanguageHandling != AlternativeMapLanguageHandling::LocalOnly)
      {
        vector<LanguageIndex> const similarMapLanguageIndexes = SimilarLanguageIndexes(mapLanguageIndex);
        prioritizedMapLanguageIndexes.insert(prioritizedMapLanguageIndexes.cend(), similarMapLanguageIndexes.cbegin(),
                                             similarMapLanguageIndexes.cend());
      }
    }
  }

  if (alternativeMapLanguageHandling == AlternativeMapLanguageHandling::LocalOnly)
  {
    LanguageIndex mapLanguageIndex = mapLanguageIndexes.front();
    prioritizedMapLanguageIndexes.push_back(mapLanguageIndex);

    vector<LanguageIndex> const similarLanguageIndexes = SimilarLanguageIndexes(mapLanguageIndex);
    prioritizedMapLanguageIndexes.insert(prioritizedMapLanguageIndexes.cend(), similarLanguageIndexes.cbegin(),
                                         similarLanguageIndexes.cend());
  }

  prioritizedMapLanguageIndexes.push_back(kInternationalNameIndex);
  prioritizedMapLanguageIndexes.push_back(kEnglishLanguageIndex);
  prioritizedMapLanguageIndexes.push_back(kDefaultNameIndex);

  return prioritizedMapLanguageIndexes;
}

}  // namespace localisation
