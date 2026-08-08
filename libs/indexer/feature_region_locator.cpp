#include "indexer/feature_region_locator.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/platform.hpp"

#include "i18n/country_language.hpp"

namespace feature
{
using namespace std;
using namespace localisation;

/// Constructor
RegionLocator::RegionLocator()
{
  m_infoGetter = storage::CountryInfoReader::CreateCountryInfoGetter(GetPlatform());
}

RegionLocator::~RegionLocator() = default;

/// Static instance
RegionLocator const & RegionLocator::Instance()
{
  static RegionLocator instance;
  return instance;
}

vector<LanguageCode> RegionLocator::GetLocalLanguageCodes(m2::PointD const point) const
{
  return CountryLanguage::Instance().GetLocalLanguageCodes(m_infoGetter->GetRegionCountryId(point));
}

vector<LanguageIndex> RegionLocator::GetLocalLanguageIndexes(m2::PointD const point) const
{
  return CountryLanguage::Instance().GetLocalLanguageIndexes(m_infoGetter->GetRegionCountryId(point));
}
}  // namespace feature
