#pragma once

#include "location_sharing_types.hpp"
#include "crypto_util.hpp"

#include "platform/location.hpp"

#include <functional>
#include <memory>
#include <string>

namespace location_sharing
{

// Callback types
using StateChangeCallback = std::function<void(SessionState)>;
using ErrorCallback = std::function<void(std::string const & error)>;
using PayloadReadyCallback = std::function<void(EncryptedPayload const & payload)>;

// Main session manager class
class LocationSharingSession
{
public:
  LocationSharingSession();
  ~LocationSharingSession();

  // Start a new sharing session
  // Returns credentials for sharing URL generation
  SessionCredentials Start(SessionConfig const & config);

  // Stop the current session
  void Stop();

  // Update location (call this when new GPS data arrives)
  void UpdateLocation(location::GpsInfo const & gpsInfo);

  // Update navigation info (call when route is active)
  void UpdateNavigationInfo(uint64_t eta, uint32_t distanceRemaining, std::string const & destinationName);

  // Clear navigation info (call when route ends)
  void ClearNavigationInfo();

  // Update battery level
  void UpdateBatteryLevel(uint8_t batteryPercent);

  // Get current state
  SessionState GetState() const { return m_state; }

  // Get current credentials
  SessionCredentials const & GetCredentials() const { return m_credentials; }

  // Get current configuration
  SessionConfig const & GetConfig() const { return m_config; }

  // Check if session is active
  bool IsActive() const { return m_state == SessionState::Active; }

  // Set callbacks
  void SetStateChangeCallback(StateChangeCallback callback) { m_stateChangeCallback = callback; }
  void SetErrorCallback(ErrorCallback callback) { m_errorCallback = callback; }
  void SetPayloadReadyCallback(PayloadReadyCallback callback) { m_payloadReadyCallback = callback; }

private:
  void SetState(SessionState newState);
  void OnError(std::string const & error);
  void ProcessLocationUpdate();
  bool ShouldSendUpdate() const;
  std::optional<EncryptedPayload> CreateEncryptedPayload() const;

  SessionState m_state = SessionState::Inactive;
  SessionCredentials m_credentials;
  SessionConfig m_config;

  // Current location data
  std::unique_ptr<LocationPayload> m_currentPayload;

  // Last update timestamp (to enforce update interval)
  uint64_t m_lastUpdateTimestamp = 0;

  // Callbacks
  StateChangeCallback m_stateChangeCallback;
  ErrorCallback m_errorCallback;
  PayloadReadyCallback m_payloadReadyCallback;
};

}  // namespace location_sharing
