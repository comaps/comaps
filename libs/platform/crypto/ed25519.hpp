#pragma once
#include <cstddef>
#include <cstdint>

namespace platform::crypto
{
bool VerifyEd25519(uint8_t const * pubKey, uint8_t const * msg, size_t msgSize, uint8_t const * sig);
}  // namespace platform::crypto
