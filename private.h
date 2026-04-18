#pragma once

#define OSM_OAUTH2_CLIENT_ID "feHq7fMSmKzZD6XLgPPah3whHPbavSTrJCgwhLMmqT0"
#define OSM_OAUTH2_REDIRECT_URI "cm://oauth2/osm/callback"
#define OSM_OAUTH2_SCOPE "read_prefs write_api write_notes"
#define MWM_GEOLOCATION_SERVER ""
#define DIFF_LIST_URL ""
#define METASERVER_URL "https://cdn-us-1.comaps.app"
#define DEFAULT_URLS_JSON R"([ "https://comaps.firewall-gateway.de/", "https://cdn-us-2.comaps.tech/", "https://cdn-fi-1.comaps.app/", "https://comaps.openstreetmap.fr/", "https://comaps-it1.unfoxo.it/", "https://comaps-cdn.s3-website.cloud.ru/", "https://mapgen-fi-1.comaps.app/" ])"
#define DEFAULT_CONNECTION_CHECK_IP "151.101.195.52"  // For now the IP of comaps.app (Fastly CDN)
#define TRAFFIC_DATA_BASE_URL ""
#define USER_BINDING_PKCS12 ""
#define USER_BINDING_PKCS12_PASSWORD ""

// The app is compatible with maps belonging to the following or earlier map series:
#define MAP_SERIES "2026.04.05"

// The public key for verifying countries.txt signature: libs/storage/countries_txt_signature.hpp
// Note: It cannot be moved here because it would break the iOS build.
