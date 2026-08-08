#include "platform/crypto/ed25519.hpp"

#include "std/target_os.hpp"

#ifdef OMIM_OS_IPHONE
#import "platform-Swift.h"
#else
#include "base/logging.hpp"
#endif

namespace platform::crypto
{
bool VerifyEd25519(uint8_t const * pubKey, uint8_t const * msg, size_t msgSize, uint8_t const * sig)
{
#ifdef OMIM_OS_IPHONE
  return [Bridge verifyRegionsFileWithRawPublicKey:[NSData dataWithBytes:pubKey length:32] rawData:[NSData dataWithBytes:msg length:msgSize] rawSignature:[NSData dataWithBytes:sig length:64]];
#else
  LOG(LWARNING, ("Ed25519 verification is not implemented on Apple platforms (apart from iOS) yet."));
  return false;
#endif
}
} // namespace platform::crypto
