#!/usr/bin/env bash

# Upload a new maps version to all CDN nodes (in parallel) and remove old versions.
# Maps are stored on CDNs as maps/<map series>/<maps version>/, i.e. maps/2026.06.28/260714/
# Versions to remove required argument because there's currently no way to guess.

set -e -u

usage() {
  echo "Usage: upload_to_cdn.sh MAPS_PATH NEW_SERIES REMOVE_6 REMOVE_3"
  echo "ex: sudo upload_to_cdn.sh osm-maps/2026_07_14__04_45_12/260714 2026.10.27 2026.04.05/260617 2026.06.28/260628"
  echo "  MAPS_PATH   local dir with the generated maps, its name is the maps version"
  echo "  NEW_SERIES  map series to upload into, must match map_series in countries.txt if present"
  echo "  REMOVE_6    SERIES/VERSION to remove from the 6-version CDNs, or 'none'"
  echo "  REMOVE_3    SERIES/VERSION to remove from the 3-version CDNs, or 'none'"
  echo "Old versions are removed first, and the new version uploads in parallel to all CDN nodes."
  echo "Subsequent runs will update only missing/differing files,"
  echo "so its fine to run second time to ensure there were no incomplete transfers"
  echo "or to run on an unfinished generation first and then again after its fully finished."
  echo "(sudo is needed to access rclone.conf with servers credentials)"
  exit 1
}

[ $# -eq 4 ] || usage

MAPS=$(basename $1)
DIR=$(dirname $1)/$MAPS
SERIES=$2
REMOVE_6=$3
REMOVE_3=$4

# Uploading into the wrong series would serve maps to apps which can't read them
if [ -f $DIR/countries.txt ] && ! grep -q "\"map_series\": *\"$SERIES\"" $DIR/countries.txt; then
  echo "ERROR: map series $SERIES doesn't match $DIR/countries.txt"
  exit 1
fi

echo "Uploading maps folder $DIR to $SERIES/$MAPS"

# Remove old versions before uploading new ones
echo "Checking for old versions to remove..."

remove_old() {
  local REMOTE_DIR="$1"
  local VERSION="$2"
  [[ "$VERSION" == "none" ]] && return
  # A malformed version could purge a whole series or all maps
  if ! [[ "$VERSION" =~ ^[0-9]{4}\.[0-9]{2}\.[0-9]{2}/[0-9]{6}$ ]] || [[ "$VERSION" == "$SERIES/$MAPS" ]]; then
    echo "ERROR: refusing to delete $REMOTE_DIR/$VERSION"
    exit 1
  fi
  echo "Deleting $REMOTE_DIR/$VERSION"
  rclone purge $REMOTE_DIR/$VERSION/ || echo "  $REMOTE_DIR/$VERSION is not present"
}

RU1="ru1:comaps-maps/maps"
remove_old $RU1 $REMOVE_3

FI1="fi1:/var/www/html/maps"
remove_old $FI1 $REMOVE_3

DE1="de1:/var/www/html/comaps-cdn/maps"
remove_old $DE1 $REMOVE_6

FR1="fr1:/data/maps"
remove_old $FR1 $REMOVE_6

IT1="it1:maps"
remove_old $IT1 $REMOVE_6

US2="us2:comaps-map-files/maps"
remove_old $US2 $REMOVE_6

echo "Old version cleanup complete"

echo "Uploading to us2"
# An explicit mwm/txt filter is used to skip temp files when run for an unfinished generation
nice rclone copy --include "*.{mwm,txt,sig}" $DIR $US2/$SERIES/$MAPS &

echo "Uploading to ru1"
nice rclone copy --include "*.{mwm,txt,sig}" $DIR $RU1/$SERIES/$MAPS &

echo "Uploading to fi1"
nice rclone copy --include "*.{mwm,txt,sig}" $DIR $FI1/$SERIES/$MAPS &

echo "Uploading to de1"
nice rclone copy --include "*.{mwm,txt,sig}" $DIR $DE1/$SERIES/$MAPS &

echo "Uploading to fr1"
nice rclone copy --include "*.{mwm,txt,sig}" $DIR $FR1/$SERIES/$MAPS &

echo "Uploading to it1"
nice rclone copy --include "*.{mwm,txt,sig}" $DIR $IT1/$SERIES/$MAPS &

# us1 is not used for maps atm
# rclone lsd us1:/home/dh_zzxxrk/cdn-us-1.comaps.app/maps

wait

echo "Running once more without parallelization to output status:"

echo "$US2 status:"
nice rclone copy -v --include "*.{mwm,txt,sig}" $DIR $US2/$SERIES/$MAPS

echo "$RU1 status:"
nice rclone copy -v --include "*.{mwm,txt,sig}" $DIR $RU1/$SERIES/$MAPS

echo "$FI1 status:"
nice rclone copy -v --include "*.{mwm,txt,sig}" $DIR $FI1/$SERIES/$MAPS

echo "$DE1 status:"
nice rclone copy -v --include "*.{mwm,txt,sig}" $DIR $DE1/$SERIES/$MAPS

echo "$FR1 status:"
nice rclone copy -v --include "*.{mwm,txt,sig}" $DIR $FR1/$SERIES/$MAPS

echo "$IT1 status:"
nice rclone copy -v --include "*.{mwm,txt,sig}" $DIR $IT1/$SERIES/$MAPS

echo "Upload complete"

echo "us2 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=2 $US2 | sort -r | tr '\n' ' ')"
echo "ru1 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=2 $RU1 | sort -r | tr '\n' ' ')"
echo "fi1 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=2 $FI1 | sort -r | tr '\n' ' ')"
echo "de1 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=2 $DE1 | sort -r | tr '\n' ' ')"
echo "fr1 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=2 $FR1 | sort -r | tr '\n' ' ')"
echo "it1 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=2 $IT1 | sort -r | tr '\n' ' ')"

echo "DONE"