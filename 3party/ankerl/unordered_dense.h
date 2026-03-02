#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace ankerl
{
namespace unordered_dense
{
template <class Key, class T, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>,
          class Alloc = std::allocator<std::pair<Key const, T>>>
using map = std::unordered_map<Key, T, Hash, Pred, Alloc>;
template <class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<Key>>
using set = std::unordered_set<Key, Hash, KeyEqual, Allocator>;
}  // namespace unordered_dense
}  // namespace ankerl
