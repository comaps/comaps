#include "platform/crypto/ed25519.hpp"
#include "base/logging.hpp"

namespace platform::crypto
{
bool VerifyEd25519(uint8_t const * pubKey, uint8_t const * msg, size_t msgSize, uint8_t const * sig)
{
  LOG(LWARNING, ("Ed25519 verification is not implemented on Apple platforms yet."));
  return false;
}
} // namespace platform::crypto
