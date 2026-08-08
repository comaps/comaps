#include "categories_holder.hpp"

#include "coding/reader.hpp"

#include "platform/platform.hpp"

#include "defines.hpp"

CategoriesHolder const & GetDefaultCategories()
{
  static CategoriesHolder const instance(GetPlatform().GetReader(SEARCH_CATEGORIES_FILE_NAME));
  return instance;
}
