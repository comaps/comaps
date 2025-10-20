#pragma once

#include "location_sharing_types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace location_sharing
{
namespace crypto
{

// AES-256-GCM encryption parameters
constexpr size_t kAesKeySize = 32;      // 256 bits
constexpr size_t kGcmIvSize = 12;       // 96 bits (recommended for GCM)
constexpr size_t kGcmAuthTagSize = 16;  // 128 bits

// Encrypt data using AES-256-GCM
// key: base64-encoded 32-byte key
// plaintext: data to encrypt
// Returns: encrypted payload with IV and auth tag, or nullopt on failure
std::optional<EncryptedPayload> EncryptAes256Gcm(
    std::string const & key,
    std::string const & plaintext);

// Decrypt data using AES-256-GCM
// key: base64-encoded 32-byte key
// payload: encrypted payload with IV and auth tag
// Returns: decrypted plaintext, or nullopt on failure/auth failure
std::optional<std::string> DecryptAes256Gcm(
    std::string const & key,
    EncryptedPayload const & payload);

// Generate a random IV (initialization vector)
std::vector<uint8_t> GenerateRandomIV();

// Generate a random AES-256 key (32 bytes)
std::vector<uint8_t> GenerateRandomKey();

}  // namespace crypto
}  // namespace location_sharing
