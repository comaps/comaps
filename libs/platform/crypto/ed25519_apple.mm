#include "platform/crypto/ed25519.hpp"
#include "base/logging.hpp"

#import "platform-Swift.h"

namespace platform::crypto
{
bool VerifyEd25519(uint8_t const * pubKey, size_t pubKeySize, uint8_t const * msg, size_t msgSize, uint8_t const * sig, size_t sigSize)
{
  return [Bridge verifyRegionsFileWithRawPublicKey:[NSData dataWithBytes:pubKey length:pubKeySize] rawData:[NSData dataWithBytes:msg length:msgSize] rawSignature:[NSData dataWithBytes:sig length:sigSize]];
}
} // namespace platform::crypto
