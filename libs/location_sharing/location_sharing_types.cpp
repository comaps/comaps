#include "location_sharing_types.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "coding/base64.hpp"

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>

namespace location_sharing
{

namespace
{
// Generate a UUID v4
std::string GenerateUUID()
{
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dis;

  uint64_t data1 = dis(gen);
  uint64_t data2 = dis(gen);

  // Set version to 4 (random UUID)
  data1 = (data1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
  // Set variant to RFC 4122
  data2 = (data2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  oss << std::setw(8) << (data1 >> 32);
  oss << '-';
  oss << std::setw(4) << ((data1 >> 16) & 0xFFFF);
  oss << '-';
  oss << std::setw(4) << (data1 & 0xFFFF);
  oss << '-';
  oss << std::setw(4) << (data2 >> 48);
  oss << '-';
  oss << std::setw(12) << (data2 & 0xFFFFFFFFFFFFULL);

  return oss.str();
}

// Generate random bytes and base64 encode
std::string GenerateRandomBase64(size_t numBytes)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint8_t> dis(0, 255);

  std::vector<uint8_t> bytes(numBytes);
  for (size_t i = 0; i < numBytes; ++i)
    bytes[i] = dis(gen);

  return base64::Encode(std::string(bytes.begin(), bytes.end()));
}

// URL-safe base64 encoding (replace +/ with -_, remove padding)
std::string ToBase64Url(std::string const & data)
{
  std::string encoded = base64::Encode(data);

  // Replace characters for URL safety
  for (char & c : encoded)
  {
    if (c == '+') c = '-';
    else if (c == '/') c = '_';
  }

  // Remove padding
  auto pos = encoded.find('=');
  if (pos != std::string::npos)
    encoded = encoded.substr(0, pos);

  return encoded;
}

// URL-safe base64 decoding
std::optional<std::string> FromBase64Url(std::string const & encoded)
{
  std::string data = encoded;

  // Reverse URL-safe substitutions
  for (char & c : data)
  {
    if (c == '-') c = '+';
    else if (c == '_') c = '/';
  }

  // Add padding if needed
  size_t padding = (4 - (data.length() % 4)) % 4;
  data.append(padding, '=');

  std::string decoded = base64::Decode(data);
  if (decoded.empty())
    return std::nullopt;

  return decoded;
}

}  // namespace

// Utility function for timestamps
uint64_t GetCurrentTimestamp()
{
  auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
}

// LocationPayload implementation

LocationPayload::LocationPayload(location::GpsInfo const & gpsInfo)
{
  timestamp = GetCurrentTimestamp();
  latitude = gpsInfo.m_latitude;
  longitude = gpsInfo.m_longitude;
  accuracy = gpsInfo.m_horizontalAccuracy;

  if (gpsInfo.m_speed > 0.0)
    speed = gpsInfo.m_speed;

  if (gpsInfo.m_bearing >= 0.0)
    bearing = gpsInfo.m_bearing;

  mode = SharingMode::Standalone;
}

std::string LocationPayload::ToJson() const
{
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(6);

  oss << "{";
  oss << "\"timestamp\":" << timestamp << ",";
  oss << "\"lat\":" << latitude << ",";
  oss << "\"lon\":" << longitude << ",";
  oss << "\"accuracy\":" << accuracy;

  if (speed.has_value())
    oss << ",\"speed\":" << *speed;

  if (bearing.has_value())
    oss << ",\"bearing\":" << *bearing;

  oss << ",\"mode\":\"" << (mode == SharingMode::Navigation ? "navigation" : "standalone") << "\"";

  if (mode == SharingMode::Navigation)
  {
    if (eta.has_value())
      oss << ",\"eta\":" << *eta;

    if (distanceRemaining.has_value())
      oss << ",\"distanceRemaining\":" << *distanceRemaining;

    if (destinationName.has_value())
      oss << ",\"destinationName\":\"" << *destinationName << "\"";
  }

  if (batteryLevel.has_value())
    oss << ",\"batteryLevel\":" << static_cast<int>(*batteryLevel);

  oss << "}";
  return oss.str();
}

std::optional<LocationPayload> LocationPayload::FromJson(std::string const & json)
{
  // Basic JSON parsing - in production, use a proper JSON library
  // This is a simplified implementation
  LOG(LWARNING, ("JSON parsing not fully implemented - placeholder"));
  return std::nullopt;
}

// EncryptedPayload implementation

std::string EncryptedPayload::ToJson() const
{
  std::ostringstream oss;
  oss << "{";
  oss << "\"iv\":\"" << iv << "\",";
  oss << "\"ciphertext\":\"" << ciphertext << "\",";
  oss << "\"authTag\":\"" << authTag << "\"";
  oss << "}";
  return oss.str();
}

std::optional<EncryptedPayload> EncryptedPayload::FromJson(std::string const & json)
{
  LOG(LWARNING, ("JSON parsing not fully implemented - placeholder"));
  return std::nullopt;
}

// SessionCredentials implementation

SessionCredentials SessionCredentials::Generate()
{
  SessionCredentials creds;
  creds.sessionId = GenerateUUID();
  creds.encryptionKey = GenerateRandomBase64(32);  // 32 bytes for AES-256
  return creds;
}

std::string SessionCredentials::GenerateShareUrl(std::string const & serverBaseUrl) const
{
  // Combine sessionId and key with separator
  std::string combined = sessionId + ":" + encryptionKey;

  // URL-safe base64 encode
  std::string encoded = ToBase64Url(combined);

  // Construct URL
  std::string url = serverBaseUrl;
  if (url.back() != '/')
    url += '/';
  url += "live/" + encoded;

  return url;
}

std::optional<SessionCredentials> SessionCredentials::ParseFromUrl(std::string const & url)
{
  // Extract the encoded part after "/live/"
  auto pos = url.find("/live/");
  if (pos == std::string::npos)
    return std::nullopt;

  std::string encoded = url.substr(pos + 6);  // 6 = length of "/live/"

  // Decode from URL-safe base64
  auto decodedOpt = FromBase64Url(encoded);
  if (!decodedOpt.has_value())
    return std::nullopt;

  std::string decoded = *decodedOpt;

  // Split on ':'
  auto colonPos = decoded.find(':');
  if (colonPos == std::string::npos)
    return std::nullopt;

  SessionCredentials creds;
  creds.sessionId = decoded.substr(0, colonPos);
  creds.encryptionKey = decoded.substr(colonPos + 1);

  return creds;
}

}  // namespace location_sharing
