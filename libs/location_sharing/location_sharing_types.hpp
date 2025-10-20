#pragma once

#include "platform/location.hpp"

#include <cstdint>
#include <string>
#include <optional>

namespace location_sharing
{

// Sharing mode determines what information is included
enum class SharingMode : uint8_t
{
  Standalone,  // GPS position only
  Navigation   // GPS + ETA + distance remaining
};

// Core location sharing payload (before encryption)
struct LocationPayload
{
  uint64_t timestamp;           // Unix timestamp in seconds
  double latitude;              // Decimal degrees
  double longitude;             // Decimal degrees
  double accuracy;              // Horizontal accuracy in meters
  std::optional<double> speed;  // Speed in m/s
  std::optional<double> bearing; // Bearing in degrees (0-360)

  SharingMode mode = SharingMode::Standalone;

  // Navigation-specific fields (only when mode == Navigation)
  std::optional<uint64_t> eta;              // Estimated time of arrival (Unix timestamp)
  std::optional<uint32_t> distanceRemaining; // Distance in meters
  std::optional<std::string> destinationName; // Optional destination name

  // Battery level (0-100) - helps viewers understand if tracking may stop
  std::optional<uint8_t> batteryLevel;

  LocationPayload() = default;

  // Construct from GpsInfo
  explicit LocationPayload(location::GpsInfo const & gpsInfo);

  // Serialize to JSON string
  std::string ToJson() const;

  // Deserialize from JSON string
  static std::optional<LocationPayload> FromJson(std::string const & json);
};

// Encrypted location payload ready for transmission
struct EncryptedPayload
{
  std::string iv;              // Base64-encoded initialization vector (12 bytes for GCM)
  std::string ciphertext;      // Base64-encoded encrypted data
  std::string authTag;         // Base64-encoded authentication tag (16 bytes for GCM)

  // Serialize to JSON for HTTP POST
  std::string ToJson() const;

  // Deserialize from JSON
  static std::optional<EncryptedPayload> FromJson(std::string const & json);
};

// Session configuration
struct SessionConfig
{
  uint32_t updateIntervalSeconds = 20;  // Default 20 seconds
  bool includeDestinationName = true;
  bool includeBatteryLevel = true;
  uint8_t lowBatteryThreshold = 10;     // Stop sharing below this percentage

  SessionConfig() = default;
};

// Session credentials
struct SessionCredentials
{
  std::string sessionId;    // UUID v4 format
  std::string encryptionKey; // 32 bytes, base64-encoded

  SessionCredentials() = default;
  SessionCredentials(std::string const & id, std::string const & key)
    : sessionId(id), encryptionKey(key) {}

  // Generate new random session credentials
  static SessionCredentials Generate();

  // Generate shareable URL
  std::string GenerateShareUrl(std::string const & serverBaseUrl) const;

  // Parse credentials from share URL
  static std::optional<SessionCredentials> ParseFromUrl(std::string const & url);
};

// Session state
enum class SessionState : uint8_t
{
  Inactive,     // Not started
  Starting,     // Initializing
  Active,       // Actively sharing
  Paused,       // Temporarily paused (e.g., app backgrounded)
  Stopping,     // Shutting down
  Error         // Error state
};

}  // namespace location_sharing
