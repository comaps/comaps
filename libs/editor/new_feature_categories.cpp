#include "new_feature_categories.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"

#include "platform/localization.hpp"
#include "base/string_utils.hpp"

#include <algorithm>

namespace osm
{
NewFeatureCategories::NewFeatureCategories(editor::EditorConfig const & config)
{
  Classificator const & c = classif();
  for (auto const & clType : config.GetTypesThatCanBeAdded())
  {
    uint32_t const type = c.GetTypeByReadableObjectName(clType);
    if (type == 0)
    {
      LOG(LWARNING, ("Unknown type in Editor's config:", clType));
      continue;
    }
    m_types.emplace_back(clType);
  }
  m_addedLangs.reserve(CategoriesHolder::kLocaleMapping.size() + 1);
}

NewFeatureCategories::NewFeatureCategories(NewFeatureCategories && other) noexcept
  : m_index(std::move(other.m_index))
  , m_types(std::move(other.m_types))
{
  // Do not move m_addedLangs, see Framework::GetEditorCategories() usage.
}

void NewFeatureCategories::AddLanguage(std::string lang)
{
  auto const & c = classif();

  for (auto const & mapping : CategoriesHolder::kLocaleMapping)
  {
    auto const langCode = mapping.m_code;
    
    // Skip if we already loaded this language
    if (m_addedLangs.contains(langCode))
      continue;

    for (auto const & type : m_types)
      m_index.AddCategoryByTypeAndLang(c.GetTypeByReadableObjectName(type), langCode);

    m_addedLangs.insert(langCode);
  }
}

NewFeatureCategories::TypeNames NewFeatureCategories::Search(std::string const & query) const
{
  std::vector<uint32_t> resultTypes;
  m_index.GetAssociatedTypes(query, resultTypes);

  auto const & c = classif();
  NewFeatureCategories::TypeNames result(resultTypes.size());
  for (size_t i = 0; i < result.size(); ++i)
    result[i] = c.GetReadableObjectName(resultTypes[i]);

  // Fallback for Regional Languages (e.g. pt-BR, es-MX)
  if (!query.empty())
  {
    auto const lowerQuery = strings::MakeLowerCase(query);
    for (auto const & typeStr : m_types)
    {
      // Skip if the core index already found it
      if (std::find(result.begin(), result.end(), typeStr) != result.end())
        continue;

      // Fetch the exact localized string used by the UI
      auto const locName = platform::GetLocalizedTypeName(typeStr);
      if (locName.empty())
        continue;

      // Perform a substring match
      if (strings::MakeLowerCase(locName).find(lowerQuery) != std::string::npos)
        result.push_back(typeStr);
    }
  }

  return result;
}

}  // namespace osm
