#!/usr/bin/env bash
# run_oa_preprocessor.sh — Download an OpenAddresses collection and run the CoMaps preprocessor.
#
# Required environment variables:
#   OA_API_TOKEN         Bearer token for batch.openaddresses.io
#   OA_COLLECTION_ID     Integer collection ID (e.g. 6 for Canada)
#   OA_COLLECTION_NAME   Collection name used in filenames/paths (e.g. ca)
#   COMAPS_DIR           Path to the CoMaps source checkout
#
# Optional environment variables:
#   OA_ZIP_PATH          Where to save the downloaded ZIP (default: /tmp/collection-oa-${OA_COLLECTION_NAME}.zip)
#   OA_OUTPUT_DIR        Output dir for .tempaddr files (default: /tmp/oa-out-${OA_COLLECTION_NAME})
#   OA_SOURCES_DIR       Local clone of github.com/openaddresses/openaddresses for offline license lookup
#   SKIP_DOWNLOAD        Set to 1 to skip the download step and use an existing ZIP

set -euo pipefail

: "${OA_API_TOKEN:?OA_API_TOKEN is required}"
: "${OA_COLLECTION_ID:?OA_COLLECTION_ID is required}"
: "${OA_COLLECTION_NAME:?OA_COLLECTION_NAME is required}"
: "${COMAPS_DIR:?COMAPS_DIR is required}"

OA_ZIP_PATH="${OA_ZIP_PATH:-/tmp/collection-oa-${OA_COLLECTION_NAME}.zip}"
OA_OUTPUT_DIR="${OA_OUTPUT_DIR:-/tmp/oa-out-${OA_COLLECTION_NAME}}"
BORDERS_DIR="${COMAPS_DIR}/data/borders"
PREPROCESSOR="${COMAPS_DIR}/tools/python/maps_generator/generator/openaddresses_preprocessor.py"
SKIP_DOWNLOAD="${SKIP_DOWNLOAD:-0}"

echo "=== OpenAddresses preprocessor for collection ${OA_COLLECTION_NAME} (ID: ${OA_COLLECTION_ID}) ==="

# --- Download ---
if [ "${SKIP_DOWNLOAD}" != "1" ]; then
    echo "Resolving download URL ..."
    # Two-step download: the API returns a 302 redirect to the actual file host.
    # We resolve the redirect URL first (without -L so %{redirect_url} is populated),
    # then download from the resolved URL without the Bearer token.
    # Sending the Authorization header directly to the file host may cause errors
    # on some S3-compatible hosts that use their own auth in the URL.
    DOWNLOAD_URL=$(curl -fsS \
        -H "Authorization: Bearer ${OA_API_TOKEN}" \
        -w "%{redirect_url}" -o /dev/null \
        "https://batch.openaddresses.io/api/collections/${OA_COLLECTION_ID}/data")

    if [ -z "${DOWNLOAD_URL}" ]; then
        echo "ERROR: could not resolve download URL for collection ${OA_COLLECTION_ID}" >&2
        exit 1
    fi

    echo "Downloading ${DOWNLOAD_URL} -> ${OA_ZIP_PATH} ..."
    [ -f "${OA_ZIP_PATH}" ] && mv -f "${OA_ZIP_PATH}" "${OA_ZIP_PATH}.bak"
    curl -fsSL -o "${OA_ZIP_PATH}" "${DOWNLOAD_URL}"
    echo "Downloaded $(du -sh "${OA_ZIP_PATH}" | cut -f1)"
else
    echo "Skipping download (SKIP_DOWNLOAD=1), using existing: ${OA_ZIP_PATH}"
    [ -f "${OA_ZIP_PATH}" ] || { echo "ERROR: ${OA_ZIP_PATH} does not exist" >&2; exit 1; }
fi

# --- Run preprocessor ---
echo "Preparing output directory: ${OA_OUTPUT_DIR} ..."
rm -rf "${OA_OUTPUT_DIR}.bak"
[ -d "${OA_OUTPUT_DIR}" ] && mv -fT "${OA_OUTPUT_DIR}" "${OA_OUTPUT_DIR}.bak" || true
mkdir -p "${OA_OUTPUT_DIR}"

PREPROCESSOR_ARGS=(
    "${OA_ZIP_PATH}"
    "${OA_OUTPUT_DIR}"
    --borders-dir "${BORDERS_DIR}"
)

if [ -n "${OA_SOURCES_DIR:-}" ]; then
    PREPROCESSOR_ARGS+=(--oa-sources-dir "${OA_SOURCES_DIR}")
fi

echo "Running preprocessor ..."
python3 "${PREPROCESSOR}" "${PREPROCESSOR_ARGS[@]}"

echo ""
echo "=== Done. Output in ${OA_OUTPUT_DIR} ==="
echo "Set ADDRESSES_PATH=${OA_OUTPUT_DIR} in the [External] section of map_generator.ini"
ls -lah "${OA_OUTPUT_DIR}" | head -20
