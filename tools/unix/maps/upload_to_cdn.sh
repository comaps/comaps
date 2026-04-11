#!/usr/bin/env bash

# Upload new maps version to all CDN nodes (in parallel) and remove old versions.

set -e -u

if [ $# -eq 0 ]; then
  echo "Usage: upload_to_cdn.sh MAPS_PATH"
  echo "e.g. sudo upload_to_cdn.sh osm-maps/2025_09_06__09_48_08/250906"
  echo "uploads are run in parallel to all CDN nodes/servers,"
  echo "subsequent runs will update only missing/differing files,"
  echo "so its fine to run second time to ensure there were no incomplete transfers"
  echo "or to run on an unfinished generation first and then again after its fully finished."
  echo "(sudo is needed to access rclone.conf with servers credentials)"
  exit 1
fi

MAPS=$(basename $1)
DIR=$(dirname $1)/$MAPS

echo "Uploading maps folder $DIR to $MAPS"

# Remove old versions before uploading new ones
echo "Checking for old versions to remove..."

remove_old() {
  local REMOTE_DIR="$1"
  local MAX="$2"
  local COUNT=0
  local VERSIONS=$(rclone lsf --dirs-only --dir-slash=false --max-depth=1 "$REMOTE_DIR" | sort -r | tr '\n' ' ')
  echo "Checking $REMOTE_DIR (keep $MAX versions)"
  echo "  Versions present: $VERSIONS"
  for version in $VERSIONS; do
    if [ $version -gt 250101 ]; then
      if [[ "$version" == "$MAPS" ]]; then
        echo "  $MAPS dir is present already, skip cleaning and continue previous upload"
        return
      fi
      COUNT=$(($COUNT + 1))
      if [ "$COUNT" -ge "$MAX" ]; then
        echo "  Deleting $version"
        rclone purge $REMOTE_DIR/$version/
        # Delete only 1 version at a time, for safety
        return
      fi
    fi
  done
}

RU1="ru1:comaps-maps/maps"
remove_old $RU1 3

FI1="fi1:/var/www/html/maps"
remove_old $FI1 3

DE1="de1:/var/www/html/comaps-cdn/maps"
remove_old $DE1 6

FR1="fr1:/data/maps"
remove_old $FR1 6

IT1="it1:maps"
remove_old $IT1 6

US2="us2:comaps-map-files/maps"
echo "Skipping $US2 cleanup (keep all versions)"
echo "  Versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=1 $US2 | sort -r | tr '\n' ' ')"

echo "Old version cleanup complete"

echo "Uploading to us2"
# An explicit mwm/txt filter is used to skip temp files when run for an unfinished generation
rclone copy --include "*.{mwm,txt,sig}" $DIR $US2/$MAPS &

echo "Uploading to ru1"
rclone copy --include "*.{mwm,txt,sig}" $DIR $RU1/$MAPS &

echo "Uploading to fi1"
rclone copy --include "*.{mwm,txt,sig}" $DIR $FI1/$MAPS &

echo "Uploading to de1"
rclone copy --include "*.{mwm,txt,sig}" $DIR $DE1/$MAPS &

echo "Uploading to fr1"
rclone copy --include "*.{mwm,txt,sig}" $DIR $FR1/$MAPS &

echo "Uploading to it1"
rclone copy --include "*.{mwm,txt,sig}" $DIR $IT1/$MAPS &

# us1 is not used for maps atm
# rclone lsd us1:/home/dh_zzxxrk/cdn-us-1.comaps.app/maps

wait

echo "Running once more without parallelization to output status:"

echo "$US2 status:"
rclone copy -v --include "*.{mwm,txt,sig}" $DIR $US2/$MAPS

echo "$RU1 status:"
rclone copy -v --include "*.{mwm,txt,sig}" $DIR $RU1/$MAPS

echo "$FI1 status:"
rclone copy -v --include "*.{mwm,txt,sig}" $DIR $FI1/$MAPS

echo "$DE1 status:"
rclone copy -v --include "*.{mwm,txt,sig}" $DIR $DE1/$MAPS

echo "$FR1 status:"
rclone copy -v --include "*.{mwm,txt,sig}" $DIR $FR1/$MAPS

echo "$IT1 status:"
rclone copy -v --include "*.{mwm,txt,sig}" $DIR $IT1/$MAPS

echo "Upload complete"

echo "us2 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=1 $US2 | sort -r | tr '\n' ' ')"
echo "ru1 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=1 $RU1 | sort -r | tr '\n' ' ')"
echo "fi1 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=1 $FI1 | sort -r | tr '\n' ' ')"
echo "de1 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=1 $DE1 | sort -r | tr '\n' ' ')"
echo "fr1 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=1 $FR1 | sort -r | tr '\n' ' ')"
echo "it1 versions present: $(rclone lsf --dirs-only --dir-slash=false --max-depth=1 $IT1 | sort -r | tr '\n' ' ')"

echo "DONE"