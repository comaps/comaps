#include "coding/sha256.hpp"

#include <boost/hash2/sha2.hpp>

#include <algorithm>

namespace coding
{
// Thin wrapper over boost
// static
SHA256::Hash SHA256::CalculateForString(std::string_view str)
{
  boost::hash2::sha2_256 hasher;
  hasher.update(str.data(), str.size());
  auto const digest = hasher.result();

  Hash result;
  std::copy_n(digest.data(), digest.size(), result.begin());
  return result;
}
}  // namespace coding
