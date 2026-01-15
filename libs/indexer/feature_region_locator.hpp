#pragma once

#include "coding/reader.hpp"
#include "cppjansson/cppjansson.hpp"
#include "storage/storage.hpp"
#include "storage/country_info_getter.hpp"

#include <vector>

namespace feature
{
  using namespace std;

  class RegionLocator
  {
  public:
    /// Static instance
    static RegionLocator const & Instance();
    
    /**
     * Find the local languages codes for a given point
     * @param point The point to check
     * @return The local language codes
     */
    vector<string> GetLocalLanguages(m2::PointD const point) const
    {
      auto const regionId = m_infoGetter->GetRegionCountryId(point);

      vector<string> regionIdParts;
      for (auto const regionIdPart : strings::Tokenize(regionId, "_")) {
        regionIdParts.push_back(string(regionIdPart));
      }

      json_t const * jsonData = nullptr;
      vector<string> languages;
      while (languages.empty() && !regionIdParts.empty()) {
        string regionId = strings::JoinStrings(regionIdParts, "_");
        FromJSONObjectOptionalField(m_jsonRoot.get(), regionId, jsonData);
        if (jsonData)
          FromJSONObjectOptionalField(jsonData, "languages", languages);
        regionIdParts.pop_back();
      }
      return languages;
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
