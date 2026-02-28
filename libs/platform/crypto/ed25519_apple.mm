#include "platform/crypto/ed25519.hpp"
#include "base/logging.hpp"

#import "platform-Swift.h"

namespace platform::crypto
{
bool VerifyEd25519(uint8_t const * pubKey, uint8_t const * msg, size_t msgSize, uint8_t const * sig)
{
  return [Bridge verifyRegionsFileWithRawPublicKey:[NSData dataWithBytes:&pubKey length:sizeof(pubKey)] rawData:[NSData dataWithBytes:&msg length:sizeof(msg)] dataSize:msgSize rawSignature:[NSData dataWithBytes:&sig length:sizeof(sig)]];
}
} // namespace platform::crypto
