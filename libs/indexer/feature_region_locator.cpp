#include "indexer/feature_region_locator.hpp"

#include "defines.hpp"
#include "platform/platform.hpp"

namespace feature
{
/// Constructor
RegionLocator::RegionLocator()
{
  auto & platform = GetPlatform();
  m_infoGetter = storage::CountryInfoReader::CreateCountryInfoGetter(platform);

  auto reader = platform.GetReader(COUNTRIES_META_FILE);
  string buffer;
  reader->ReadAsString(buffer);
  m_jsonRoot = base::Json(buffer.data());
}

/// Static instance
RegionLocator const & RegionLocator::Instance()
{
  static RegionLocator instance;
  return instance;
}
}  // namespace feature
