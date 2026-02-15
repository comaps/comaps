#pragma once

#include <climits>

#if SIZE_MAX > 4294967295
#include "__flat_hash_map.hpp"
#else
#include <functional>
#include <memory>
#include <utility>

#include "3party/ankerl/unordered_dense.h"

namespace ska
{
template <class Key, class T, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>,
          class Alloc = std::allocator<std::pair<const Key, T>>>
using flat_hash_map = ankerl::unordered_dense::map<Key, T, Hash, Pred, Alloc>;
}  // namespace ska
#endif
