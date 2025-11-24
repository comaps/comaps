#pragma once

#include "indexer/classificator.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/macros.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>

#include "3party/ankerl/unordered_dense.h"

namespace search
{
class CategoriesSet
{
public:
  CategoriesSet() : m_classificator(classif()) {}

  inline void Add(uint32_t type) { m_categories.insert(type); }

  template <typename Fn>
  void ForEach(Fn && fn) const
  {
    std::for_each(m_categories.begin(), m_categories.end(), std::forward<Fn>(fn));
  }

private:
  Classificator const & m_classificator;
  ankerl::unordered_dense::set<uint32_t> m_categories;

  DISALLOW_COPY_AND_MOVE(CategoriesSet);
};
}  // namespace search
