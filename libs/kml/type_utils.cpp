#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_region_locator.hpp"
#include "indexer/feature_utils.hpp"

#include "platform/localization.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/string_utf8_multilang.hpp"

namespace kml
{
bool IsEqual(m2::PointD const & lhs, m2::PointD const & rhs)
{
  return lhs.EqualDxDy(rhs, kMwmPointAccuracy);
}

bool IsEqual(geometry::PointWithAltitude const & lhs, geometry::PointWithAltitude const & rhs)
{
  return AlmostEqualAbs(lhs, rhs, kMwmPointAccuracy);
}

std::string GetPreferredBookmarkStrForKml(LocalizableString const & name, std::vector<localisation::LanguageIndex> const languageIndexes)
{
  if (name.size() == 1)
    return name.begin()->second;

  /// @todo Complicated logic here when transforming LocalizableString -> StringUtf8Multilang to call GetPreferredName.
  StringUtf8Multilang nameMultilang;
  for (auto const & pair : name)
    nameMultilang.AddString(pair.first, pair.second);

  std::optional<std::string> translatedName = localisation::TranslatedFeatureName(nameMultilang, languageIndexes).m_primary;
  if (translatedName.has_value())
    return translatedName.value();

  return {};
}

std::string GetLocalizedFeatureType(std::vector<uint32_t> const & types)
{
  if (types.empty())
    return {};

  auto const & c = classif();
  auto const type = c.GetTypeForIndex(types.front());

  return localisation::TranslatedFeatureType(c.GetReadableObjectName(type));
}

std::string GetPreferredBookmarkNameForKml(BookmarkData const & bmData)
{
  std::vector<localisation::LanguageIndex> languageIndexes = feature::RegionLocator::Instance().GetLocalLanguageIndexes(bmData.m_point);
  std::string name = GetPreferredBookmarkStrForKml(bmData.m_customName, languageIndexes);
  if (name.empty())
    name = GetPreferredBookmarkStrForKml(bmData.m_name, languageIndexes);
  if (name.empty())
    name = GetLocalizedFeatureType(bmData.m_featureTypes);
  return name;
}
}  // namespace kml
