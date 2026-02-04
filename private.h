#pragma once
#include <array>
#include <cstdint>

#define OSM_OAUTH2_CLIENT_ID "feHq7fMSmKzZD6XLgPPah3whHPbavSTrJCgwhLMmqT0"
#define OSM_OAUTH2_REDIRECT_URI "cm://oauth2/osm/callback"
#define OSM_OAUTH2_SCOPE "read_prefs write_api write_notes"
#define MWM_GEOLOCATION_SERVER ""
#define DIFF_LIST_URL ""
#define METASERVER_URL "https://cdn-us-1.comaps.app/servers"
#define DEFAULT_URLS_JSON R"([ "https://cdn-us-2.comaps.tech/", "https://comaps.firewall-gateway.de/", "https://cdn-fi-1.comaps.app/", "https://comaps-cdn.s3-website.cloud.ru/", "https://mapgen-fi-1.comaps.app/" ])"
#define DEFAULT_CONNECTION_CHECK_IP "151.101.195.52"  // For now the IP of comaps.app (Fastly CDN)
#define TRAFFIC_DATA_BASE_URL ""
#define USER_BINDING_PKCS12 ""
#define USER_BINDING_PKCS12_PASSWORD ""
#define MIN_COMPAT_APP_V "2026.02.09-4"

namespace storage
{
inline constexpr std::array<uint8_t, 32> kCountriesTxtPublicKey = {
    0x91, 0xc0, 0xa9, 0xf6, 0xaa, 0x18, 0x23, 0x71, 0xf0, 0x47, 0xe2, 0x56, 0xab, 0x46, 0x48, 0x92,
    0x11, 0xac, 0xc2, 0xb5, 0x1b, 0x13, 0x19, 0x7f, 0xbe, 0x8c, 0x94, 0xea, 0xa9, 0x74, 0x9c, 0x7b};
}  // namespace storage
