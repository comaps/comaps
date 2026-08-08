#!/usr/bin/env bash
#
# Please run this script to configure the repository after cloning it.
#

echo "Configuring the repository for development..."

export PYTHONUTF8=1

SKIP_MAP_DOWNLOAD="${SKIP_MAP_DOWNLOAD:-}"
SKIP_GENERATE_SYMBOLS="${SKIP_GENERATE_SYMBOLS:-}"
SKIP_GENERATE_DRULES="${SKIP_GENERATE_DRULES:-}"
SKIP_GENERATE_JSON_STRINGS="${SKIP_GENERATE_JSON_STRINGS:-}"
SKIP_GENERATE_STRINGS="${SKIP_GENERATE_STRINGS:-}"
SKIP_GENERATE_SERBIAN_LATIN_STRINGS="${SKIP_GENERATE_SERBIAN_LATIN_STRINGS:-}"

DRULES_NOT_GENERATED=
SYMBOLS_NOT_GENERATED=
STRINGS_NOT_GENERATED=

DRULES_FILES=(drules_proto.bin drules_proto_default_dark.bin drules_proto_default_light.bin drules_proto_outdoors_dark.bin drules_proto_outdoors_light.bin drules_proto_vehicle_dark.bin drules_proto_vehicle_light.bin classificator.txt types.txt visibility.txt colors.txt patterns.txt)
SYMBOLS_FILES=(xhdpi/light/symbols.png xhdpi/light/symbols.sdf xhdpi/dark/symbols.png xhdpi/dark/symbols.sdf mdpi/light/symbols.png mdpi/light/symbols.sdf mdpi/dark/symbols.png mdpi/dark/symbols.sdf 6plus/light/symbols.png 6plus/light/symbols.sdf 6plus/dark/symbols.png 6plus/dark/symbols.sdf xxxhdpi/light/symbols.png xxxhdpi/light/symbols.sdf xxxhdpi/dark/symbols.png xxxhdpi/dark/symbols.sdf hdpi/light/symbols.png hdpi/light/symbols.sdf hdpi/dark/symbols.png hdpi/dark/symbols.sdf xxhdpi/light/symbols.png xxhdpi/light/symbols.sdf xxhdpi/dark/symbols.png xxhdpi/dark/symbols.sdf)

for f in ${DRULES_FILES[*]}; do
  if [ ! -f "data/$f" ]; then
    DRULES_NOT_GENERATED=1
    break
  fi
done

for f in ${SYMBOLS_FILES[*]}; do
  if [ ! -f "data/symbols/$f" ]; then
    SYMBOLS_NOT_GENERATED=1
    break
  fi
done

if [ ! -f "libs/platform/localized_types_map.cpp" ]; then
  STRINGS_NOT_GENERATED=1
fi

############################# PROCESS OPTIONS ################################

TEMP=$(getopt -o ms --long skip-map-download,skip-generate-symbols,skip-generate-drules \
              -n 'configure' -- "$@")

if [ $? != 0 ] ; then echo "Terminating..." >&2 ; exit 1 ; fi

eval set -- "$TEMP"

while true; do
  case "$1" in
    -m | --skip-map-download ) SKIP_MAP_DOWNLOAD=1; shift ;;
    -s | --skip-generate-symbols ) SKIP_GENERATE_SYMBOLS=1; shift ;;
    -d | --skip-generate-drules ) SKIP_GENERATE_DRULES=1; shift ;;
    -O | --skip-generate-json-strings ) SKIP_GENERATE_JSON_STRINGS=1; shift ;;
    -S | --skip-generate-strings ) SKIP_GENERATE_STRINGS=1; shift ;;
    -L | --skip-generate-serbian-latin-strings ) SKIP_GENERATE_SERBIAN_LATIN_STRINGS=1; shift ;;
    * ) break ;;
  esac
done

# Shift the processed options away
shift $((OPTIND-1))

set -euo pipefail

###############################################################################

pushd() { command pushd "$@" > /dev/null; }
popd() { command popd "$@" > /dev/null; }

Diff() {
  local HASH_PATH=$1
  shift
  local HASH="$(md5sum "$@" | md5sum)"
  if [ "$HASH" != "$(cat "$HASH_PATH" 2>/dev/null)" ]; then
    printf "$HASH" > "$HASH_PATH"
  else
    false
  fi
}


echo "Checking submodules..."
git submodule update --init --recursive --depth 1

if [ ! -d 3party/boost/boost ]; then
  echo "Bootstrapping the boost C++ library..."
  pushd 3party/boost/
  ./bootstrap.sh
  ./b2 headers
  popd
fi

# Provision and activate a local Python virtual environment with protobuf
source ./tools/unix/activate_venv.sh

if [ -z "$SKIP_MAP_DOWNLOAD" ]; then
  pushd data

  MWM_VERSION=$(awk -F'[:,]' '/"v":/{ $2 = substr($2, 2); print $2 }' countries.txt)
  # Maps are found under this location
  # https://mapgen-fi-1.comaps.app/maps/<map_series>/<version>/World.mwm
  MAP_SERIES=$(awk -F'"' '/"map_series":/{ print $4; exit }' countries.txt)
  MAPS_BASE_URL="https://mapgen-fi-1.comaps.app/maps/$MAP_SERIES/$MWM_VERSION"
  MWM_PATH="world_mwm/$MWM_VERSION"
  WORLD_PATH="$MWM_PATH/World.mwm"
  WORLD_PATH2="$MWM_PATH/WorldCoasts.mwm"

  mkdir -p "$MWM_PATH"

  # TODO: if needed World map file version exists already then we need to update a symlink to point to it anyway
  if [ ! -f "$WORLD_PATH" ]; then
    echo "Downloading world map..."
    # mapgen-fi-1 is supposed to have all historic prod map versions as well as recent test maps
    if ! wget -N "$MAPS_BASE_URL/World.mwm" -P "$MWM_PATH"; then
      echo "ERROR: could not download World.mwm from $MAPS_BASE_URL" >&2
      exit 1
    fi
    rm -f World.mwm; ln -s "$WORLD_PATH" World.mwm
  fi
  if [ ! -f "$WORLD_PATH2" ]; then
    if ! wget -N "$MAPS_BASE_URL/WorldCoasts.mwm" -P "$MWM_PATH"; then
      echo "ERROR: could not download WorldCoasts.mwm from $MAPS_BASE_URL" >&2
      exit 1
    fi
    rm -f WorldCoasts.mwm; ln -s "$WORLD_PATH2" WorldCoasts.mwm
  fi

  if [ ! -f "World.mwm" ]; then
    ln -s "$WORLD_PATH" World.mwm
  fi
  if [ ! -f "WorldCoasts.mwm" ]; then
    ln -s "$WORLD_PATH2" WorldCoasts.mwm
  fi

  popd
else
  echo "Skipping world map download..."
fi

# This step must be before all the other strings steps, since they expect strings in data/
if [ -z "$SKIP_GENERATE_JSON_STRINGS" ]; then
  echo "Generating json strings..."
  ./tools/unix/generate_json_strings.sh
else
  echo "Skipping generate json strings..."
fi

echo "Generating search categories / synonyms..."
./tools/unix/generate_categories.sh

if [ -z "$SKIP_GENERATE_STRINGS" ]; then
  if Diff data/strings_hash iphone/Maps/LocalizedStrings/en.lproj/LocalizableTypes.strings || [ ! -z "$STRINGS_NOT_GENERATED" ]; then
    echo "Generating Desktop UI strings..."
    ./tools/unix/generate_desktop_ui_strings.sh
  fi
else
  echo "Skipping generate Desktop UI strings..."
fi

if [ -z "$SKIP_GENERATE_SERBIAN_LATIN_STRINGS" ]; then
  echo "Generating Serbian Latin strings..."
  ./tools/unix/generate_serbian_latin_strings.sh
else
  echo "Skipping generate Serbian Latin strings..."
fi

if [ -z "$SKIP_GENERATE_SYMBOLS" ]; then
  if Diff data/symbols_hash data/styles/*/*/symbols/* || [ ! -z "$SYMBOLS_NOT_GENERATED" ]; then
    echo "Generating symbols..."
    bash ./tools/unix/generate_symbols.sh || (rm data/symbols_hash; exit 1)
  fi
else
  echo "Skipping generate symbols..."
fi

if [ -z "$SKIP_GENERATE_DRULES" ]; then
  if Diff data/drules_hash data/styles/*/*/*.mapcss data/styles/*/*/*.prio.txt data/mapcss-mapping.csv || [ ! -z "$DRULES_NOT_GENERATED" ]; then
    echo "Generating drules..."
    bash ./tools/unix/generate_drules.sh || (rm data/drules_hash; exit 1)
  fi
else
  echo "Skipping generate drules..."
fi

echo "The repository is configured for development."
