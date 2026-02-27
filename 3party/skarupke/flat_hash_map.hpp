#pragma once

#include <climits>

#if SIZE_MAX > 4294967295
#include "__flat_hash_map.hpp"
#else
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

namespace ska
{
template <class Key, class T, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>,
          class Alloc = std::allocator<std::pair<const Key, T>>>
// TODO(x7z4w): ankerl::unordered_dense::map doesn't work here on 32-bit builds, use it here once fixed
using flat_hash_map = std::unordered_map<Key, T, Hash, Pred, Alloc>;
}  // namespace ska
#endif
