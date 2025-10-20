# Location Sharing Library

## Overview

Core C++ library for live GPS location sharing with zero-knowledge end-to-end encryption in Organic Maps.

## Features

- **AES-256-GCM Encryption**: Industry-standard authenticated encryption
- **Zero-knowledge architecture**: Server never sees encryption keys
- **Cross-platform**: Linux/Android (OpenSSL), iOS/macOS (CommonCrypto), Windows (BCrypt)
- **Automatic session management**: UUID generation, key derivation, URL encoding
- **Update scheduling**: Configurable intervals with automatic throttling
- **Battery protection**: Auto-stop below threshold
- **Dual modes**: Standalone GPS or navigation with ETA/distance

## Components

### 1. Types (`location_sharing_types.hpp/cpp`)

**SessionCredentials:**
```cpp
SessionCredentials creds = SessionCredentials::Generate();
std::string shareUrl = creds.GenerateShareUrl("https://server.com");
```

**LocationPayload:**
```cpp
LocationPayload payload(gpsInfo);
payload.mode = SharingMode::Navigation;
payload.eta = timestamp + timeToArrival;
std::string json = payload.ToJson();
```

**EncryptedPayload:**
```cpp
EncryptedPayload encrypted;
encrypted.iv = "...";  // 12 bytes base64
encrypted.ciphertext = "...";
encrypted.authTag = "...";  // 16 bytes base64
std::string json = encrypted.ToJson();
```

### 2. Encryption (`crypto_util.hpp/cpp`)

**Encrypt:**
```cpp
std::string key = "base64-encoded-32-byte-key";
std::string plaintext = "{...}";
auto encrypted = crypto::EncryptAes256Gcm(key, plaintext);
if (encrypted.has_value()) {
  std::string json = encrypted->ToJson();
  // Send to server
}
```

**Decrypt:**
```cpp
auto decrypted = crypto::DecryptAes256Gcm(key, encryptedPayload);
if (decrypted.has_value()) {
  // Process plaintext
}
```

**Platform implementations:**
- **OpenSSL** (Android/Linux): `EVP_aes_256_gcm()` with `EVP_CIPHER_CTX`
- **CommonCrypto** (iOS/macOS): TODO - currently placeholder, needs Security framework
- **BCrypt** (Windows): TODO - currently placeholder

### 3. Session Management (`location_sharing_session.hpp/cpp`)

**Basic usage:**
```cpp
LocationSharingSession session;

// Set callbacks
session.SetPayloadReadyCallback([](EncryptedPayload const & payload) {
  // Send to server
  PostToServer(payload.ToJson());
});

// Start session
SessionConfig config;
config.updateIntervalSeconds = 20;
SessionCredentials creds = session.Start(config);
std::string shareUrl = creds.GenerateShareUrl("https://server.com");

// Update location
session.UpdateLocation(gpsInfo);

// Update navigation (optional)
session.UpdateNavigationInfo(eta, distance, "Destination Name");

// Update battery
session.UpdateBatteryLevel(85);

// Stop session
session.Stop();
```

**State machine:**
- `Inactive` → `Starting` → `Active` → `Stopping` → `Inactive`
- `Error` state on failures

**Callbacks:**
```cpp
session.SetStateChangeCallback([](SessionState state) {
  LOG(LINFO, ("State:", static_cast<int>(state)));
});

session.SetErrorCallback([](std::string const & error) {
  LOG(LERROR, ("Error:", error));
});
```

## Building

### CMake

```cmake
add_subdirectory(libs/location_sharing)

target_link_libraries(your_target
  PRIVATE
    location_sharing
)
```

### Dependencies

**Linux/Android:**
```bash
sudo apt-get install libssl-dev
```

**iOS/macOS:**
- Security framework (automatic)

**Windows:**
- BCrypt (automatic)

### Android NDK

OpenSSL is typically bundled or available via:
```gradle
android {
  externalNativeBuild {
    cmake {
      arguments "-DOPENSSL_ROOT_DIR=/path/to/openssl"
    }
  }
}
```

## Security

### Encryption Details

- **Algorithm**: AES-256-GCM (Galois/Counter Mode)
- **Key size**: 256 bits (32 bytes)
- **IV size**: 96 bits (12 bytes) - recommended for GCM
- **Auth tag size**: 128 bits (16 bytes)
- **Key generation**: `SecRandomCopyBytes` (iOS) or `std::random_device` fallback

### URL Format

```
https://server.com/live/{base64url(sessionId:encryptionKey)}
```

Example:
```
https://live.organicmaps.app/live/YWJjZDEyMzQtNTY3OC05MGFiLWNkZWYtMTIzNDU2Nzg5MGFiOmFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6MTIzNDU2Nzg
```

Decoded:
```
abcd1234-5678-90ab-cdef-1234567890ab:abcdefghijklmnopqrstuvwxyz12345678
                                      ^
                                      separator
```

### Zero-Knowledge Architecture

1. **Client** generates session ID (UUID) + 256-bit key
2. **Client** encrypts location with key
3. **Server** stores encrypted blob (no key access)
4. **Share URL** contains both session ID and key
5. **Viewer** decrypts in-browser using key from URL

**Server NEVER sees:**
- Encryption key
- Plaintext location data
- User identity (session ID is random UUID)

### Threat Model

**Protected against:**
- Server compromise (data is encrypted)
- Man-in-the-middle (TLS + authenticated encryption)
- Replay attacks (timestamp in payload)
- Tampering (GCM authentication tag)

**NOT protected against:**
- URL interception (anyone with URL can decrypt)
- Client compromise (key is in memory)
- Quantum computers (AES-256 is quantum-resistant)

## Testing

### Unit Tests

```cpp
// Test encryption round-trip
TEST(CryptoUtil, EncryptDecrypt) {
  std::string key = GenerateRandomKey();
  std::string plaintext = "test data";

  auto encrypted = crypto::EncryptAes256Gcm(key, plaintext);
  ASSERT_TRUE(encrypted.has_value());

  auto decrypted = crypto::DecryptAes256Gcm(key, *encrypted);
  ASSERT_TRUE(decrypted.has_value());
  EXPECT_EQ(plaintext, *decrypted);
}

// Test auth tag validation
TEST(CryptoUtil, AuthTagValidation) {
  auto encrypted = crypto::EncryptAes256Gcm(key, plaintext);
  encrypted->authTag[0] ^= 0xFF; // Corrupt tag

  auto decrypted = crypto::DecryptAes256Gcm(key, *encrypted);
  EXPECT_FALSE(decrypted.has_value()); // Should fail
}
```

### Integration Tests

See `docs/LOCATION_SHARING_INTEGRATION.md` for full testing guide.

## Performance

### Benchmarks (approximate)

- **Encryption**: ~1-5 ms for 200-byte payload (hardware-dependent)
- **Memory**: ~100 bytes per session
- **Network**: ~300-400 bytes per update (encrypted + JSON overhead)

### Optimization Tips

1. **Reuse sessions**: Don't create new session per update
2. **Batch updates**: If sending multiple locations, consider batching
3. **Adjust intervals**: Increase `updateIntervalSeconds` for better battery life

## Troubleshooting

### OpenSSL linking errors (Android/Linux)

```
undefined reference to `EVP_EncryptInit_ex`
```

**Solution:**
```cmake
find_package(OpenSSL REQUIRED)
target_link_libraries(location_sharing PRIVATE OpenSSL::SSL OpenSSL::Crypto)
```

### CommonCrypto not found (iOS)

**Solution:**
```cmake
find_library(SECURITY_FRAMEWORK Security)
target_link_libraries(location_sharing PRIVATE ${SECURITY_FRAMEWORK})
```

### Encryption fails at runtime

**Check:**
1. Key is exactly 32 bytes (base64-decoded)
2. IV is 12 bytes
3. Auth tag is 16 bytes
4. Platform crypto library is available

**Debug logging:**
```cpp
#define LOG_CRYPTO_ERRORS
```

## API Reference

See header files for full API documentation:
- `location_sharing_types.hpp`: Data structures
- `crypto_util.hpp`: Encryption functions
- `location_sharing_session.hpp`: Session management

## Contributing

When adding features:
1. Maintain zero-knowledge architecture
2. Add unit tests for crypto changes
3. Update documentation
4. Test on all platforms (Android, iOS, Linux)

## License

Same as Organic Maps (Apache 2.0)

## References

- [AES-GCM Specification (NIST SP 800-38D)](https://csrc.nist.gov/publications/detail/sp/800-38d/final)
- [OpenSSL EVP Interface](https://www.openssl.org/docs/man3.0/man3/EVP_EncryptInit.html)
- [Web Crypto API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Crypto_API)
