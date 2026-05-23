#include "i18n/country_language.hpp"

#include "coding/reader.hpp"

#include "platform/platform.hpp"

#include "base/string_utils.hpp"

#include "defines.hpp"

namespace localisation
{
using namespace std;

/// Constructor
CountryLanguage::CountryLanguage()
{
  auto reader = GetPlatform().GetReader(COUNTRIES_META_FILE);
  string buffer;
  reader->ReadAsString(buffer);
  m_jsonRoot = base::Json(buffer.data());
}

/// Static instance
CountryLanguage const & CountryLanguage::Instance()
{
  static CountryLanguage instance;
  return instance;
}

vector<LanguageCode> CountryLanguage::GetLocalLanguageCodes(string_view regionId) const
{
  vector<string> regionIdParts;
  for (auto const regionIdPart : strings::Tokenize(regionId, "_"))
    regionIdParts.push_back(string(regionIdPart));

  json_t const * jsonData = nullptr;
  vector<LanguageCode> languages;
  while (languages.empty() && !regionIdParts.empty())
  {
    string regionId = strings::JoinStrings(regionIdParts, "_");
    FromJSONObjectOptionalField(m_jsonRoot.get(), regionId, jsonData);
    if (jsonData)
      FromJSONObjectOptionalField(jsonData, "languages", languages);
    regionIdParts.pop_back();
  }
  return languages;
}

vector<LanguageIndex> CountryLanguage::GetLocalLanguageIndexes(string_view regionId) const
{
  vector<string> regionIdParts;
  for (auto const regionIdPart : strings::Tokenize(regionId, "_"))
    regionIdParts.push_back(string(regionIdPart));

  json_t const * jsonData = nullptr;
  vector<LanguageCode> languageCodes;
  while (languageCodes.empty() && !regionIdParts.empty())
  {
    string regionId = strings::JoinStrings(regionIdParts, "_");
    FromJSONObjectOptionalField(m_jsonRoot.get(), regionId, jsonData);
    if (jsonData)
      FromJSONObjectOptionalField(jsonData, "languages", languageCodes);
    regionIdParts.pop_back();
  }
  return ConvertLanguageCodesToLanguageIndexes(languageCodes);
}
}  // namespace localisation
