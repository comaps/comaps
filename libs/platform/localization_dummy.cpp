#include <ctime>
#include "platform/localization.hpp"
#include "indexer/localized_types_map.cpp"

namespace platform
{
std::string GetLocalizedTypeName(std::string const & type)
{
  auto key = "type." + type;
  std::replace(key.begin(), key.end(), '-', '.');
  std::replace(key.begin(), key.end(), ':', '_');
  auto const it = g_type2localizedType.find(key);
  std::string localizedName = (it != g_type2localizedType.end()) ? it->second : std::string();
  return localizedName.empty() ? type : localizedName;
}

std::string GetLocalizedBrandName(std::string const & brand)
{
  return brand;
}

std::string GetLocalizedString(std::string const & key)
{
  return key;
}

std::string GetCurrencySymbol(std::string const & currencyCode)
{
  return currencyCode;
}

std::string GetLocalizedMyPositionBookmarkName()
{
  std::time_t t = std::time(nullptr);
  char buf[100] = {0};
  (void)std::strftime(buf, sizeof(buf), "%Ec", std::localtime(&t));
  return buf;
}
}  // namespace platform
