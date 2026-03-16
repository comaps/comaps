#pragma once

#include "editor/editor_config.hpp"

#include "base/macros.hpp"

#include <string>
#include <vector>

#include "3party/ankerl/unordered_dense.h"

namespace osm
{
// This class holds an index of categories that can be set for a newly added feature.
class NewFeatureCategories
{
public:
  using TypeName = std::string;
  using TypeNames = std::vector<TypeName>;

  NewFeatureCategories() = default;
  explicit NewFeatureCategories(editor::EditorConfig const & config);

  NewFeatureCategories(NewFeatureCategories && other) noexcept;
  NewFeatureCategories & operator=(NewFeatureCategories && other) = default;

  // Adds all known synonyms in language |lang| for all categories that
  // can be applied to a newly added feature.
  void AddLanguage(std::string const & lang);

  // Returns names and types of categories that have a synonym containing
  // the substring |query| (in any language that was added before).
  // The returned list is sorted.
  TypeNames Search(std::string const & query) const;

  // Returns all registered classifier category types (GetReadableObjectName).
  TypeNames const & GetAllCreatableTypeNames() const { return m_types; }

private:
  struct CategoryData
  {
    TypeName m_typeName;
    std::vector<std::string> m_synonyms;
  };

  TypeNames m_types;
  std::vector<CategoryData> m_categoriesData;
  ankerl::unordered_dense::set<std::string> m_baseLangs;

  DISALLOW_COPY(NewFeatureCategories);
};
}  // namespace osm
