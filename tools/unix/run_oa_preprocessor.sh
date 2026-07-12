#!/usr/bin/env bash
# run_oa_preprocessor.sh — Download an OpenAddresses collection, extract addresses
# as TSV, then run the C++ oa_processor_tool to write .tempaddr files.
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
SKIP_DOWNLOAD="${SKIP_DOWNLOAD:-0}"

# Path to oa_processor_tool.  Override via OA_PROCESSOR_TOOL if it's not on PATH
# (e.g. when built in a non-standard location like omim-build-release/).
OA_PROCESSOR_TOOL="${OA_PROCESSOR_TOOL:-oa_processor_tool}"

PREPROCESSOR="${COMAPS_DIR}/tools/python/maps_generator/generator/openaddresses_preprocessor.py"

echo "=== OpenAddresses preprocessor for collection ${OA_COLLECTION_NAME} (ID: ${OA_COLLECTION_ID}) ==="

# --- Download ---
if [ "${SKIP_DOWNLOAD}" != "1" ]; then
    echo "Resolving download URL ..."
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

# --- Run pipeline: Python (TSV) | C++ (.tempaddr) ---
echo "Preparing output directory: ${OA_OUTPUT_DIR} ..."
rm -rf "${OA_OUTPUT_DIR}.bak"
[ -d "${OA_OUTPUT_DIR}" ] && mv -fT "${OA_OUTPUT_DIR}" "${OA_OUTPUT_DIR}.bak" || true
mkdir -p "${OA_OUTPUT_DIR}"

PYTHON_ARGS=("${OA_ZIP_PATH}")
if [ -n "${OA_SOURCES_DIR:-}" ]; then
    PYTHON_ARGS+=(--oa-sources-dir "${OA_SOURCES_DIR}")
fi

echo "Running preprocessor pipeline ..."
python3 "${PREPROCESSOR}" "${PYTHON_ARGS[@]}" \
    | "${OA_PROCESSOR_TOOL}" --data_path "${COMAPS_DIR}/data" --output_path "${OA_OUTPUT_DIR}"

echo ""
echo "=== Done. Output in ${OA_OUTPUT_DIR} ==="
echo "Set ADDRESSES_PATH=${OA_OUTPUT_DIR} in the [External] section of map_generator.ini"
ls -lah "${OA_OUTPUT_DIR}" | head -20
