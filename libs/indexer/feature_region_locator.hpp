#pragma once

#include "coding/reader.hpp"

#include "cppjansson/cppjansson.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "base/localisation_translation.hpp"

#include <vector>

namespace feature
{
using namespace std;
using namespace localisation;

class RegionLocator
{
public:
  /// Static instance
  static RegionLocator const & Instance();

  /**
   * Find the local languages codes for a region id
   * @param regionId The region id to check
   * @return The local language codes
   */
  vector<LanguageCode> GetLocalLanguageCodes(string const regionId) const
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

  /**
   * Find the local languages codes for a given point
   * @param point The point to check
   * @return The local language codes
   */
  vector<LanguageCode> GetLocalLanguageCodes(m2::PointD const point) const
  {
    return GetLocalLanguageCodes(m_infoGetter->GetRegionCountryId(point));
  }

  /**
   * Find the local languages codes for a region id
   * @param regionId The region id to check
   * @return The local language codes
   */
  vector<LanguageIndex> GetLocalLanguageIndexes(string const regionId) const
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

  /**
   * Find the local languages codes for a given point
   * @param point The point to check
   * @return The local language codes
   */
  vector<LanguageIndex> GetLocalLanguageIndexes(m2::PointD const point) const
  {
    return GetLocalLanguageIndexes(m_infoGetter->GetRegionCountryId(point));
  }

private:
  /// Constructor
  RegionLocator();

  /// Country info getter
  unique_ptr<storage::CountryInfoGetter> m_infoGetter;

  /// JSON root
  base::Json m_jsonRoot;
};
}  // namespace feature
