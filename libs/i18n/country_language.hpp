#pragma once

#include "i18n/localisation.hpp"

#include "cppjansson/cppjansson.hpp"

#include <string_view>
#include <vector>

namespace localisation
{
class CountryLanguage
{
public:
  /// Static instance
  static CountryLanguage const & Instance();

  /**
   * Find the local languages codes for a region id
   * @param regionId The region id to check
   * @return The local language codes
   */
  std::vector<LanguageCode> GetLocalLanguageCodes(std::string_view regionId) const;

  /**
   * Find the local languages codes for a region id
   * @param regionId The region id to check
   * @return The local language codes
   */
  std::vector<LanguageIndex> GetLocalLanguageIndexes(std::string_view regionId) const;

private:
  /// Constructor
  CountryLanguage();

  /// JSON root
  base::Json m_jsonRoot;
};
}  // namespace localisation
