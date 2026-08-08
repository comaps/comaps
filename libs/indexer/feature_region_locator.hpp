#pragma once

#include "geometry/point2d.hpp"

#include "i18n/localisation.hpp"

#include <memory>
#include <vector>

namespace storage
{
class CountryInfoGetter;
}  // namespace storage

namespace feature
{
class RegionLocator
{
public:
  ~RegionLocator();

  /// Static instance
  static RegionLocator const & Instance();

  /**
   * Find the local languages codes for a given point
   * @param point The point to check
   * @return The local language codes
   */
  std::vector<localisation::LanguageCode> GetLocalLanguageCodes(m2::PointD const point) const;

  /**
   * Find the local languages codes for a given point
   * @param point The point to check
   * @return The local language codes
   */
  std::vector<localisation::LanguageIndex> GetLocalLanguageIndexes(m2::PointD const point) const;

private:
  /// Constructor
  RegionLocator();

  /// Country info getter
  std::unique_ptr<storage::CountryInfoGetter> m_infoGetter;
};
}  // namespace feature
