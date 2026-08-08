#include "i18n/localisation_translation.hpp"

#include "platform/localization.hpp"

#include "i18n/get_text_by_id.hpp"
#include "i18n/string_utf8_multilang.hpp"
#include "i18n/transliteration.hpp"

namespace localisation
{
using namespace std;

LanguageIndex LikelyRegionalLanguageIndexForRendering(vector<LanguageIndex> const regionalLanguageIndexes)
{
  if (regionalLanguageIndexes.empty())
    return kUnsupportedLanguageIndex;

  return regionalLanguageIndexes.front();
}

Translation BestTranslation(StringUtf8Multilang const translations,
                            vector<LanguageIndex> const prioritizedMapLanguageIndexes,
                            vector<LanguageIndex> const regionalLanguageIndexes)
{
  string bestTranslation = {};
  LanguageIndex bestLanguageIndex = kUnsupportedLanguageIndex;
  for (LanguageIndex const prioritizedMapLanguageIndex : prioritizedMapLanguageIndexes)
  {
    if (translations.GetString(prioritizedMapLanguageIndex, bestTranslation))
    {
      bestLanguageIndex = prioritizedMapLanguageIndex;
      break;
    }
  }

  if (bestLanguageIndex != kUnsupportedLanguageIndex && ShouldUseTransliteration())
  {
    if (bestLanguageIndex == kDefaultNameIndex)
    {
      bool shouldTransliterate = true;
      vector<LanguageIndex> const mapLanguageIndexes = GetMapLanguageIndexes();
      for (LanguageIndex const regionalLanguageIndex : regionalLanguageIndexes)
        if (!shouldTransliterate || find(mapLanguageIndexes.begin(), mapLanguageIndexes.end(),
                                              regionalLanguageIndex) != mapLanguageIndexes.end())
          shouldTransliterate = false;
      if (shouldTransliterate)
        bestTranslation = Transliteration::Instance().Transliterate(bestLanguageIndex, bestTranslation);
    }
    else if (find(regionalLanguageIndexes.begin(), regionalLanguageIndexes.end(), bestLanguageIndex) ==
             regionalLanguageIndexes.end())
    {
      bestTranslation = Transliteration::Instance().Transliterate(bestLanguageIndex, bestTranslation);
    }
  }

  if (bestLanguageIndex == kDefaultNameIndex)
    return Translation(bestTranslation, LikelyRegionalLanguageIndexForRendering(regionalLanguageIndexes));
  return Translation(bestTranslation, bestLanguageIndex);
}

Translation LocalTranslation(StringUtf8Multilang const translations,
                             vector<LanguageIndex> const regionalLanguageIndexes)
{
  string localTranslation = {};
  translations.GetString(kDefaultNameIndex, localTranslation);
  return Translation(localTranslation, LikelyRegionalLanguageIndexForRendering(regionalLanguageIndexes));
}

struct NameTranslation TranslatedFeatureName(StringUtf8Multilang const names,
                                             vector<LanguageIndex> const regionalLanguageIndexes)
{
  vector<LanguageIndex> const prioritizedMapLanguageIndexes = PrioritizedMapLanguageIndexes(regionalLanguageIndexes);

  Translation const bestName = BestTranslation(names, prioritizedMapLanguageIndexes, regionalLanguageIndexes);
  Translation const localName = LocalTranslation(names, regionalLanguageIndexes);
  if (bestName.m_translation == localName.m_translation || find(regionalLanguageIndexes.begin(), regionalLanguageIndexes.end(), bestName.m_likelyLanguageIndex) != regionalLanguageIndexes.end())
    return NameTranslation(bestName.m_translation, bestName.m_likelyLanguageIndex);
  else
    return NameTranslation(bestName.m_translation, bestName.m_likelyLanguageIndex, localName.m_translation,
                           localName.m_likelyLanguageIndex);
}

string TranslatedFeatureType(string const translationKey)
{
  return platform::GetLocalizedTypeName(translationKey);
}

string TranslatedRegionName(storage::CountryId const countryId)
{
  auto nameGetter = platform::GetTextByIdFactoryForRegion(platform::TextSource::Countries, countryId);

  if (!nameGetter)
    return {};

  string shortName = (*nameGetter)(countryId + " Short");
  if (!shortName.empty())
    return shortName;

  string officialName = (*nameGetter)(countryId);
  if (!officialName.empty())
    return officialName;

  return {};
}

string TranslatedBrand(string const translationKey)
{
  return platform::GetLocalizedBrandName(translationKey);
}

string TranslatedInterfaceText(string const translationKey)
{
  return platform::GetLocalizedString(translationKey);
}

}  // namespace localisation
