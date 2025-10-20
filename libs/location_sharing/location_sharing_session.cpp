#include "location_sharing_session.hpp"

#include "base/logging.hpp"

#include <chrono>

namespace location_sharing
{

// Forward declare from location_sharing_types.cpp
uint64_t GetCurrentTimestamp();

LocationSharingSession::LocationSharingSession() = default;

LocationSharingSession::~LocationSharingSession()
{
  if (IsActive())
    Stop();
}

SessionCredentials LocationSharingSession::Start(SessionConfig const & config)
{
  if (m_state != SessionState::Inactive)
  {
    LOG(LWARNING, ("Session already active, stopping previous session"));
    Stop();
  }

  SetState(SessionState::Starting);

  m_config = config;
  m_credentials = SessionCredentials::Generate();
  m_currentPayload = std::make_unique<LocationPayload>();
  m_lastUpdateTimestamp = 0;

  LOG(LINFO, ("Location sharing session started, ID:", m_credentials.sessionId));

  SetState(SessionState::Active);

  return m_credentials;
}

void LocationSharingSession::Stop()
{
  if (m_state == SessionState::Inactive)
    return;

  SetState(SessionState::Stopping);

  LOG(LINFO, ("Location sharing session stopped"));

  m_currentPayload.reset();
  m_credentials = SessionCredentials();
  m_lastUpdateTimestamp = 0;

  SetState(SessionState::Inactive);
}

void LocationSharingSession::UpdateLocation(location::GpsInfo const & gpsInfo)
{
  if (!IsActive())
  {
    LOG(LWARNING, ("Cannot update location - session not active"));
    return;
  }

  if (!m_currentPayload)
  {
    m_currentPayload = std::make_unique<LocationPayload>();
  }

  // Update location data
  m_currentPayload->timestamp = GetCurrentTimestamp();
  m_currentPayload->latitude = gpsInfo.m_latitude;
  m_currentPayload->longitude = gpsInfo.m_longitude;
  m_currentPayload->accuracy = gpsInfo.m_horizontalAccuracy;

  if (gpsInfo.m_speed > 0.0)
    m_currentPayload->speed = gpsInfo.m_speed;
  else
    m_currentPayload->speed = std::nullopt;

  if (gpsInfo.m_bearing >= 0.0)
    m_currentPayload->bearing = gpsInfo.m_bearing;
  else
    m_currentPayload->bearing = std::nullopt;

  ProcessLocationUpdate();
}

void LocationSharingSession::UpdateNavigationInfo(
    uint64_t eta,
    uint32_t distanceRemaining,
    std::string const & destinationName)
{
  if (!IsActive() || !m_currentPayload)
    return;

  m_currentPayload->mode = SharingMode::Navigation;
  m_currentPayload->eta = eta;
  m_currentPayload->distanceRemaining = distanceRemaining;

  if (m_config.includeDestinationName && !destinationName.empty())
    m_currentPayload->destinationName = destinationName;
}

void LocationSharingSession::ClearNavigationInfo()
{
  if (!m_currentPayload)
    return;

  m_currentPayload->mode = SharingMode::Standalone;
  m_currentPayload->eta = std::nullopt;
  m_currentPayload->distanceRemaining = std::nullopt;
  m_currentPayload->destinationName = std::nullopt;
}

void LocationSharingSession::UpdateBatteryLevel(uint8_t batteryPercent)
{
  if (!IsActive() || !m_currentPayload)
    return;

  if (m_config.includeBatteryLevel)
    m_currentPayload->batteryLevel = batteryPercent;

  // Check if battery is too low
  if (batteryPercent < m_config.lowBatteryThreshold)
  {
    LOG(LINFO, ("Battery level too low (", static_cast<int>(batteryPercent), "%), stopping location sharing"));
    OnError("Battery level too low");
    Stop();
  }
}

void LocationSharingSession::SetState(SessionState newState)
{
  if (m_state == newState)
    return;

  SessionState oldState = m_state;
  m_state = newState;

  LOG(LINFO, ("Location sharing state changed:", static_cast<int>(oldState), "->", static_cast<int>(newState)));

  if (m_stateChangeCallback)
    m_stateChangeCallback(newState);
}

void LocationSharingSession::OnError(std::string const & error)
{
  LOG(LERROR, ("Location sharing error:", error));

  if (m_errorCallback)
    m_errorCallback(error);
}

void LocationSharingSession::ProcessLocationUpdate()
{
  if (!ShouldSendUpdate())
    return;

  auto encryptedPayload = CreateEncryptedPayload();
  if (!encryptedPayload.has_value())
  {
    OnError("Failed to create encrypted payload");
    return;
  }

  m_lastUpdateTimestamp = GetCurrentTimestamp();

  if (m_payloadReadyCallback)
    m_payloadReadyCallback(*encryptedPayload);
}

bool LocationSharingSession::ShouldSendUpdate() const
{
  if (!m_currentPayload)
    return false;

  uint64_t now = GetCurrentTimestamp();
  uint64_t timeSinceLastUpdate = now - m_lastUpdateTimestamp;

  return timeSinceLastUpdate >= m_config.updateIntervalSeconds;
}

std::optional<EncryptedPayload> LocationSharingSession::CreateEncryptedPayload() const
{
  if (!m_currentPayload)
  {
    LOG(LERROR, ("No payload to encrypt"));
    return std::nullopt;
  }

  std::string json = m_currentPayload->ToJson();

  auto encrypted = crypto::EncryptAes256Gcm(m_credentials.encryptionKey, json);
  if (!encrypted.has_value())
  {
    LOG(LERROR, ("Encryption failed"));
    return std::nullopt;
  }

  return encrypted;
}

}  // namespace location_sharing
