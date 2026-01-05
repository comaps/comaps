#include "3party/monocypher/monocypher-ed25519.h"
#include "3party/monocypher/monocypher.h"
#include "ed25519.hpp"

namespace platform::crypto
{
bool VerifyEd25519(uint8_t const * pubKey, uint8_t const * msg, size_t msgSize, uint8_t const * sig)
{
  return crypto_ed25519_check(sig, pubKey, msg, msgSize) == 0;
}
}  // namespace platform::crypto
