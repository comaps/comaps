#pragma once
#include <cstddef>
#include <cstdint>

namespace platform::crypto
{
bool VerifyEd25519(uint8_t const * pubKey, size_t pubKeySize, uint8_t const * msg, size_t msgSize, uint8_t const * sig, size_t sigSize);
}  // namespace platform::crypto
