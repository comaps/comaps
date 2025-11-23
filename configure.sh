#!/usr/bin/env bash
#
# Please run this script to configure the repository after cloning it.
#

echo "Configuring the repository for development..."

SKIP_MAP_DOWNLOAD="${SKIP_MAP_DOWNLOAD:-}"
SKIP_GENERATE_SYMBOLS="${SKIP_GENERATE_SYMBOLS:-}"
SKIP_GENERATE_DRULES="${SKIP_GENERATE_DRULES:-}"

DRULES_NOT_GENERATED=
SYMBOLS_NOT_GENERATED=

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

if [ -z "$SKIP_MAP_DOWNLOAD" ]; then
  pushd data

  MWM_VERSION=$(awk -F'[:,]' '/"v":/{ $2 = substr($2, 2); print $2 }' countries.txt)
  MWM_PATH="world_mwm/$MWM_VERSION"
  WORLD_PATH="$MWM_PATH/World.mwm"
  WORLD_PATH2="$MWM_PATH/WorldCoasts.mwm"

  mkdir -p "$MWM_PATH"

  if [ ! -f "$WORLD_PATH" ]; then
    echo "Downloading world map..."
    # Using a fi1 maps mirror/CDN
    wget -N "https://cdn-fi-1.comaps.app/maps/$MWM_VERSION/World.mwm" -P "$MWM_PATH" &&
    rm -f World.mwm; ln -s "$WORLD_PATH" World.mwm
  fi
  if [ ! -f "$WORLD_PATH2" ]; then
    wget -N "https://cdn-fi-1.comaps.app/maps/$MWM_VERSION/WorldCoasts.mwm" -P "$MWM_PATH" &&
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

echo "Generating search categories / synonyms..."
./tools/unix/generate_categories.sh

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
