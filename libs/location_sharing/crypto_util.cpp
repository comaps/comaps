#include "crypto_util.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "coding/base64.hpp"

#include <random>

// Platform-specific crypto includes
#if defined(__APPLE__)
  #include <CommonCrypto/CommonCrypto.h>
#elif defined(__linux__) && !defined(__ANDROID__)
  #include <openssl/evp.h>
  #include <openssl/rand.h>
  #include <openssl/err.h>
#elif defined(_WIN32)
  #include <windows.h>
  #include <bcrypt.h>
  #pragma comment(lib, "bcrypt.lib")
#endif

namespace location_sharing
{
namespace crypto
{

namespace
{

#if defined(__APPLE__)
// Apple CommonCrypto implementation
bool EncryptAes256GcmApple(
    std::vector<uint8_t> const & key,
    std::vector<uint8_t> const & iv,
    std::vector<uint8_t> const & plaintext,
    std::vector<uint8_t> & ciphertext,
    std::vector<uint8_t> & authTag)
{
  // Note: CommonCrypto doesn't directly support GCM mode in older versions
  // This is a simplified placeholder - production code should use Security framework
  // or a third-party library like libsodium
  LOG(LWARNING, ("CommonCrypto GCM implementation is a placeholder"));
  return false;
}

bool DecryptAes256GcmApple(
    std::vector<uint8_t> const & key,
    std::vector<uint8_t> const & iv,
    std::vector<uint8_t> const & ciphertext,
    std::vector<uint8_t> const & authTag,
    std::vector<uint8_t> & plaintext)
{
  LOG(LWARNING, ("CommonCrypto GCM implementation is a placeholder"));
  return false;
}

#elif defined(__linux__) && !defined(__ANDROID__)
// OpenSSL implementation (Linux desktop only)
bool EncryptAes256GcmOpenSSL(
    std::vector<uint8_t> const & key,
    std::vector<uint8_t> const & iv,
    std::vector<uint8_t> const & plaintext,
    std::vector<uint8_t> & ciphertext,
    std::vector<uint8_t> & authTag)
{
  EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
  if (!ctx)
  {
    LOG(LERROR, ("Failed to create cipher context"));
    return false;
  }

  bool success = false;

  do
  {
    // Initialize encryption
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1)
    {
      LOG(LERROR, ("EVP_EncryptInit_ex failed"));
      break;
    }

    // Allocate output buffer
    ciphertext.resize(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    int len = 0;
    int ciphertext_len = 0;

    // Encrypt plaintext
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size()) != 1)
    {
      LOG(LERROR, ("EVP_EncryptUpdate failed"));
      break;
    }
    ciphertext_len = len;

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1)
    {
      LOG(LERROR, ("EVP_EncryptFinal_ex failed"));
      break;
    }
    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);

    // Get authentication tag
    authTag.resize(kGcmAuthTagSize);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, kGcmAuthTagSize, authTag.data()) != 1)
    {
      LOG(LERROR, ("Failed to get auth tag"));
      break;
    }

    success = true;
  } while (false);

  EVP_CIPHER_CTX_free(ctx);
  return success;
}

bool DecryptAes256GcmOpenSSL(
    std::vector<uint8_t> const & key,
    std::vector<uint8_t> const & iv,
    std::vector<uint8_t> const & ciphertext,
    std::vector<uint8_t> const & authTag,
    std::vector<uint8_t> & plaintext)
{
  EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
  if (!ctx)
  {
    LOG(LERROR, ("Failed to create cipher context"));
    return false;
  }

  bool success = false;

  do
  {
    // Initialize decryption
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1)
    {
      LOG(LERROR, ("EVP_DecryptInit_ex failed"));
      break;
    }

    // Allocate output buffer
    plaintext.resize(ciphertext.size());
    int len = 0;
    int plaintext_len = 0;

    // Decrypt ciphertext
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size()) != 1)
    {
      LOG(LERROR, ("EVP_DecryptUpdate failed"));
      break;
    }
    plaintext_len = len;

    // Set authentication tag
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, authTag.size(),
                            const_cast<uint8_t*>(authTag.data())) != 1)
    {
      LOG(LERROR, ("Failed to set auth tag"));
      break;
    }

    // Finalize decryption (will fail if auth tag doesn't match)
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1)
    {
      LOG(LERROR, ("EVP_DecryptFinal_ex failed - authentication failed"));
      break;
    }
    plaintext_len += len;
    plaintext.resize(plaintext_len);

    success = true;
  } while (false);

  EVP_CIPHER_CTX_free(ctx);
  return success;
}

#elif defined(__ANDROID__)
// Android stub - encryption not implemented yet, returns dummy data
// TODO: Implement proper AES-256-GCM using javax.crypto.Cipher via JNI
bool EncryptAes256GcmAndroid(
    std::vector<uint8_t> const & key,
    std::vector<uint8_t> const & iv,
    std::vector<uint8_t> const & plaintext,
    std::vector<uint8_t> & ciphertext,
    std::vector<uint8_t> & authTag)
{
  LOG(LWARNING, ("Android AES-GCM not implemented - using placeholder"));
  // For now, just copy plaintext to ciphertext
  ciphertext = plaintext;
  authTag.resize(kGcmAuthTagSize, 0);
  return true;
}

bool DecryptAes256GcmAndroid(
    std::vector<uint8_t> const & key,
    std::vector<uint8_t> const & iv,
    std::vector<uint8_t> const & ciphertext,
    std::vector<uint8_t> const & authTag,
    std::vector<uint8_t> & plaintext)
{
  LOG(LWARNING, ("Android AES-GCM not implemented - using placeholder"));
  // For now, just copy ciphertext to plaintext
  plaintext = ciphertext;
  return true;
}

#elif defined(_WIN32)
// Windows BCrypt implementation
bool EncryptAes256GcmWindows(
    std::vector<uint8_t> const & key,
    std::vector<uint8_t> const & iv,
    std::vector<uint8_t> const & plaintext,
    std::vector<uint8_t> & ciphertext,
    std::vector<uint8_t> & authTag)
{
  LOG(LWARNING, ("Windows BCrypt GCM implementation is a placeholder"));
  return false;
}

bool DecryptAes256GcmWindows(
    std::vector<uint8_t> const & key,
    std::vector<uint8_t> const & iv,
    std::vector<uint8_t> const & ciphertext,
    std::vector<uint8_t> const & authTag,
    std::vector<uint8_t> & plaintext)
{
  LOG(LWARNING, ("Windows BCrypt GCM implementation is a placeholder"));
  return false;
}
#endif

}  // namespace

std::vector<uint8_t> GenerateRandomIV()
{
  std::vector<uint8_t> iv(kGcmIvSize);

#if defined(__linux__) && !defined(__ANDROID__)
  if (RAND_bytes(iv.data(), iv.size()) != 1)
  {
    LOG(LERROR, ("RAND_bytes failed"));
    // Fallback to std::random_device
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    for (auto & byte : iv)
      byte = dis(gen);
  }
#else
  // Fallback for other platforms (including Android)
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> dis(0, 255);
  for (auto & byte : iv)
    byte = dis(gen);
#endif

  return iv;
}

std::vector<uint8_t> GenerateRandomKey()
{
  std::vector<uint8_t> key(kAesKeySize);

#if defined(__linux__) && !defined(__ANDROID__)
  if (RAND_bytes(key.data(), key.size()) != 1)
  {
    LOG(LERROR, ("RAND_bytes failed"));
    // Fallback
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dis(0, 255);
    for (auto & byte : key)
      byte = dis(gen);
  }
#else
  // Fallback for other platforms (including Android)
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> dis(0, 255);
  for (auto & byte : key)
    byte = dis(gen);
#endif

  return key;
}

std::optional<EncryptedPayload> EncryptAes256Gcm(
    std::string const & keyBase64,
    std::string const & plaintext)
{
  // Decode key from base64
  std::string keyData = base64::Decode(keyBase64);
  if (keyData.empty())
  {
    LOG(LERROR, ("Failed to decode key from base64"));
    return std::nullopt;
  }

  if (keyData.size() != kAesKeySize)
  {
    LOG(LERROR, ("Invalid key size:", keyData.size()));
    return std::nullopt;
  }

  std::vector<uint8_t> key(keyData.begin(), keyData.end());
  std::vector<uint8_t> iv = GenerateRandomIV();
  std::vector<uint8_t> plaintextVec(plaintext.begin(), plaintext.end());
  std::vector<uint8_t> ciphertext;
  std::vector<uint8_t> authTag;

  bool success = false;

#if defined(__APPLE__)
  success = EncryptAes256GcmApple(key, iv, plaintextVec, ciphertext, authTag);
#elif defined(__ANDROID__)
  success = EncryptAes256GcmAndroid(key, iv, plaintextVec, ciphertext, authTag);
#elif defined(__linux__)
  success = EncryptAes256GcmOpenSSL(key, iv, plaintextVec, ciphertext, authTag);
#elif defined(_WIN32)
  success = EncryptAes256GcmWindows(key, iv, plaintextVec, ciphertext, authTag);
#endif

  if (!success)
  {
    LOG(LERROR, ("Encryption failed"));
    return std::nullopt;
  }

  EncryptedPayload payload;
  payload.iv = base64::Encode(std::string(iv.begin(), iv.end()));
  payload.ciphertext = base64::Encode(std::string(ciphertext.begin(), ciphertext.end()));
  payload.authTag = base64::Encode(std::string(authTag.begin(), authTag.end()));

  return payload;
}

std::optional<std::string> DecryptAes256Gcm(
    std::string const & keyBase64,
    EncryptedPayload const & payload)
{
  // Decode key, IV, ciphertext, and auth tag from base64
  std::string keyData = base64::Decode(keyBase64);
  std::string ivData = base64::Decode(payload.iv);
  std::string ciphertextData = base64::Decode(payload.ciphertext);
  std::string authTagData = base64::Decode(payload.authTag);

  if (keyData.empty() || ivData.empty() || ciphertextData.empty() || authTagData.empty())
  {
    LOG(LERROR, ("Failed to decode base64 data"));
    return std::nullopt;
  }

  if (keyData.size() != kAesKeySize || ivData.size() != kGcmIvSize || authTagData.size() != kGcmAuthTagSize)
  {
    LOG(LERROR, ("Invalid data sizes"));
    return std::nullopt;
  }

  std::vector<uint8_t> key(keyData.begin(), keyData.end());
  std::vector<uint8_t> iv(ivData.begin(), ivData.end());
  std::vector<uint8_t> ciphertext(ciphertextData.begin(), ciphertextData.end());
  std::vector<uint8_t> authTag(authTagData.begin(), authTagData.end());
  std::vector<uint8_t> plaintext;

  bool success = false;

#if defined(__APPLE__)
  success = DecryptAes256GcmApple(key, iv, ciphertext, authTag, plaintext);
#elif defined(__ANDROID__)
  success = DecryptAes256GcmAndroid(key, iv, ciphertext, authTag, plaintext);
#elif defined(__linux__)
  success = DecryptAes256GcmOpenSSL(key, iv, ciphertext, authTag, plaintext);
#elif defined(_WIN32)
  success = DecryptAes256GcmWindows(key, iv, ciphertext, authTag, plaintext);
#endif

  if (!success)
  {
    LOG(LERROR, ("Decryption failed"));
    return std::nullopt;
  }

  return std::string(plaintext.begin(), plaintext.end());
}

}  // namespace crypto
}  // namespace location_sharing
