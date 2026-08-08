#pragma once

#include "i18n/localisation.hpp"

#include <string>
#include <vector>

class StringUtf8Multilang;

namespace storage
{
using CountryId = std::string;
}

namespace localisation
{
using namespace std;

Translation BestTranslation(StringUtf8Multilang const translations,
                            vector<LanguageIndex> const prioritizedMapLanguageIndexes);
Translation LocalTranslation(StringUtf8Multilang const translations);
struct NameTranslation TranslatedFeatureName(StringUtf8Multilang const names,
                                             vector<LanguageIndex> const regionalLanguageIndexes = {});
string TranslatedFeatureType(string const translationKey);
string TranslatedRegionName(storage::CountryId const countryId);
string TranslatedBrand(string const translationKey);
string TranslatedInterfaceText(string const translationKey);

}  // namespace localisation
