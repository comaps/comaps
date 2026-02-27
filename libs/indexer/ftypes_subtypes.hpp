#pragma once

#include "indexer/classificator.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_set>
#include <vector>

namespace ftypes
{
using namespace std;

class Subtypes
{
public:
  /// Static instance
  static Subtypes const & Instance();

  /**
   * Lists all types with subtypes
   * @return All types with subtypes
   */
  unordered_set<uint32_t> AllTypesWithSubtypes() const { return m_types; }

  /**
   * Lists all subtypes
   * @return All subtypes
   */
  unordered_set<uint32_t> AllSubtypes() const { return m_subtypes; }

  /**
   * Checks if the given type is a type with subtypes or a subtype
   * @param type The type to check
   * @return `true` if it is a type with subtypes or a subtype, otherwise `false`
   */
  bool IsTypeWithSubtypesOrSubtype(uint32_t const type) const { return IsTypeWithSubtypes(type) || IsSubtype(type); }

  /**
   * Checks if the given type is a type with subtypes
   * @param type The type to check
   * @return `true` if it is a type with subtypes, otherwise `false`
   */
  bool IsTypeWithSubtypes(uint32_t const type) const
  {
    return ranges::find(m_types.begin(), m_types.end(), type) != m_types.end();
  }

  /**
   * Checks if the given type is a subtype
   * @param type The type to check
   * @return `true` if it is a subtype, otherwise `false`
   */
  bool IsSubtype(uint32_t const type) const
  {
    return ranges::find(m_subtypes.begin(), m_subtypes.end(), type) != m_subtypes.end();
  }

  /**
   * Checks if the given type is a subtype of a given parent type
   * @param type The type to check
   * @param parentType The possible parent type
   * @return `true` if it is a subtype of the parent type, otherwise `false`
   */
  bool IsSubtypeOfParentType(uint32_t const type, uint32_t const parentType) const
  {
    auto position = m_typesWithSubtypes.find(parentType);
    if (position != m_typesWithSubtypes.end())
    {
      vector<uint32_t> subtypes = position->second;
      return ranges::find(subtypes.begin(), subtypes.end(), type) != subtypes.end();
    }

    return false;
  }

  /**
   * Compares to given types based on their type relation
   * @param firstType The first type to compare
   * @param secondType The type to compare
   * @return `true` if the first type is a subtype but the second one isn't, `false` if it is the other way around
   */
  optional<bool> ComparisonResultBasedOnTypeRelation(uint32_t const firstType, uint32_t const secondType) const
  {
    bool const firstTypeIsTypeWithSubtypes = IsTypeWithSubtypes(firstType);
    bool const firstTypeIsSubtype = IsSubtype(firstType);
    bool const secondTypeIsTypeWithSubtypes = IsTypeWithSubtypes(secondType);
    bool const secondTypeIsSubtype = IsSubtype(secondType);
    if ((!firstTypeIsTypeWithSubtypes && !firstTypeIsSubtype) ||
        (!secondTypeIsTypeWithSubtypes && !secondTypeIsSubtype) ||
        (firstTypeIsTypeWithSubtypes && secondTypeIsTypeWithSubtypes))
      return {};
    else if (firstTypeIsSubtype && secondTypeIsTypeWithSubtypes)
      return false;
    else if (firstTypeIsTypeWithSubtypes && secondTypeIsSubtype)
      return true;

    // If they got to here, both are subtypes. So use the order of the subtypes for the comparison.
    for (auto [types, subtypes] : m_typesWithSubtypes)
    {
      for (auto const subtype : subtypes)
        if (subtype == firstType)
          return true;
        else if (subtype == secondType)
          return false;
    }

    return {};
  }

  /**
   * Checks if the given type path belongs to a type with subtypes or a subtype
   * @param typePath The type path to check
   * @return `true` if it is a type with subtypes or a subtype, otherwise `false`
   */
  bool IsTypeWithSubtypesOrSubtype(vector<string> const typePath) const
  {
    return ranges::find(m_typesAndSubtypesPaths.begin(), m_typesAndSubtypesPaths.end(), typePath) !=
           m_typesAndSubtypesPaths.end();
  }

  /**
   * Compares to given types based on their type relation
   * @param firstTypePath The first type to compare
   * @param secondTypePath The type to compare
   * @return `true` if the first type is a subtype but the second one isn't, `false` if it is the other way around
   */
  optional<bool> ComparisonResultBasedOnTypeRelation(vector<string> const firstTypePath,
                                                     vector<string> const secondTypePath) const
  {
    auto const & classificator = classif();
    uint32_t const firstType =
        classificator.GetTypeByPathSafe(vector<string_view>(firstTypePath.begin(), firstTypePath.end()));
    uint32_t const secondType =
        classificator.GetTypeByPathSafe(vector<string_view>(secondTypePath.begin(), secondTypePath.end()));
    if (firstType != IndexAndTypeMapping::INVALID_TYPE && secondType != IndexAndTypeMapping::INVALID_TYPE)
      return ComparisonResultBasedOnTypeRelation(firstType, secondType);

    return {};
  }

private:
  /// Constructor
  Subtypes();

  /// Types, which have subtypes, as unordered set for faster check performance
  unordered_set<uint32_t> m_types;

  /// Subypes as unordered set for faster check performance
  unordered_set<uint32_t> m_subtypes;

  /// Types with their associated subtypes
  map<uint32_t, vector<uint32_t>> m_typesWithSubtypes;

  /// Paths of types, which have subtypes, and subtypes for the generator
  vector<vector<string>> m_typesAndSubtypesPaths;
};
}  // namespace ftypes
