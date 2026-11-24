#include "coding/sha256.hpp"

namespace coding
{
// Stub implementation, replaced by the real SHA-256 in the follow-up commit.
// Returns a zero hash so the NIST known-answer tests fail until the algorithm lands.
// static
SHA256::Hash SHA256::CalculateForString(std::string_view str)
{
  SHA256::Hash result{};
  return result;
}
}  // namespace coding
