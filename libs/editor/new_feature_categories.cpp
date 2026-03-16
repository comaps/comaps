#include "new_feature_categories.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/search_string_utils.hpp"

#include "platform/localization.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>

namespace osm
{
NewFeatureCategories::NewFeatureCategories(editor::EditorConfig const & config)
{
  auto const & c = classif();
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
}

NewFeatureCategories::NewFeatureCategories(NewFeatureCategories && other) noexcept
  : m_types(std::move(other.m_types))
  , m_categoriesData(std::move(other.m_categoriesData))
  , m_baseLangs(std::move(other.m_baseLangs))
{
}

void NewFeatureCategories::AddLanguage(std::string const & lang)
{
  std::string lowerLang = lang;
  strings::AsciiToLower(lowerLang);
  std::replace(lowerLang.begin(), lowerLang.end(), '_', '-');

  // Extract base language (e.g., "en-AU" -> "en") to load dialect variations together
  std::string const baseLang = lowerLang.substr(0, lowerLang.find('-'));

  bool anyAdded = false;
  if (m_baseLangs.insert(baseLang).second)
    anyAdded = true;

  // Always ensure English serves as a fallback.
  if (m_baseLangs.insert("en").second)
    anyAdded = true;

  if (!anyAdded)
    return; // This language group was already processed

  m_categoriesData.clear();
  m_categoriesData.reserve(m_types.size());

  auto const & c = classif();
  auto const & holder = GetDefaultCategories();

  for (auto const & typeName : m_types)
  {
    uint32_t const type = c.GetTypeByReadableObjectName(typeName);
    if (type == 0)
      continue;

    CategoryData data;
    data.m_typeName = typeName;

    auto const addSynonym = [&data](std::string const & s)
    {
      auto norm = strings::ToUtf8(search::NormalizeAndSimplifyString(s));
      if (!norm.empty())
        data.m_synonyms.push_back(std::move(norm));
    };

    // 1. Add primary localized name (from types_strings.txt via platform)
    addSynonym(platform::GetLocalizedTypeName(typeName));

    // 2. Add all dialect variants efficiently via existing CategoriesHolder mappings
    holder.ForEachNameByType(type, [&](auto const & name) {
      // name.m_locale comes safely from CategoriesHolder's internal mapping.
      std::string l(CategoriesHolder::MapIntegerToLocale(name.m_locale));
      strings::AsciiToLower(l);
      
      // Match the base language (e.g., if locale is "pt-br", nameBase is "pt")
      std::string const nameBase = l.substr(0, l.find('-'));
      if (m_baseLangs.contains(nameBase))
        addSynonym(name.m_name);
    });

    // Strip out duplicate normalized strings to maximize search performance
    std::sort(data.m_synonyms.begin(), data.m_synonyms.end());
    data.m_synonyms.erase(std::unique(data.m_synonyms.begin(), data.m_synonyms.end()), data.m_synonyms.end());

    m_categoriesData.push_back(std::move(data));
  }
}

NewFeatureCategories::TypeNames NewFeatureCategories::Search(std::string const & query) const
{
  if (query.empty())
    return {};

  std::vector<std::string> tokens;
  search::ForEachNormalizedToken(query, [&](strings::UniString const & token) {
    tokens.push_back(strings::ToUtf8(token));
  });

  if (tokens.empty())
    return {};

  TypeNames result;

  for (auto const & data : m_categoriesData)
  {
    for (auto const & syn : data.m_synonyms)
    {
      bool allTokensFound = true;
      for (auto const & token : tokens)
      {
        if (syn.find(token) == std::string::npos)
        {
          allTokensFound = false;
          break;
        }
      }
      
      if (allTokensFound)
      {
        result.push_back(data.m_typeName);
        break;
      }
    }
  }

  std::sort(result.begin(), result.end());
  return result;
}

}  // namespace osm
