#!/usr/bin/env bash

# Upload new maps version to all CDN nodes (in parallel) and remove old versions.

# Use following commands for deleting older maps:
#
# ru1 - keep max 3 maps versions
# First list all maps versions on the server
#   sudo rclone lsd ru1:comaps-maps/maps
# Delete the old version
#   sudo rclone purge -v ru1:comaps-maps/maps/250713/
#
# fi1 - max 3 versions
#   sudo rclone lsd fi1:/var/www/html/maps
#   sudo rclone purge -v fi1:/var/www/html/maps/250713/
#
# de1 - max 6 versions
#    sudo rclone lsd de1:/var/www/html/comaps-cdn/maps
#    sudo rclone purge -v de1:/var/www/html/comaps-cdn/maps/250713/
#
# us2 - all versions, don't delete
#    sudo rclone lsd us2:comaps-map-files/maps

set -e -u

if [ $# -eq 0 ]; then
  echo "Usage: upload_to_cdn.sh MAPS_PATH"
  echo "e.g. sudo upload_to_cdn.sh osm-maps/2025_09_06__09_48_08/250906"
  echo "uploads are run in parallel to us2,ru1,fi1,de1 servers,"
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

# ru1 - keep max 3 versions
echo "Cleaning ru1 (keeping 3 newest versions)..."
OLD_VERSIONS_RU1=$(rclone lsd ru1:comaps-maps/maps --max-depth 1 | awk '{print $5}' | sort -r | tail -n +4)
for version in $OLD_VERSIONS_RU1; do
  if [ $version -gt 250101 ]; then
    echo "  Deleting ru1:comaps-maps/maps/$version/"
    rclone purge -v ru1:comaps-maps/maps/$version/
  fi
done

# fi1 - keep max 3 versions
echo "Cleaning fi1 (keeping 3 newest versions)..."
OLD_VERSIONS_FI1=$(rclone lsd fi1:/var/www/html/maps --max-depth 1 | awk '{print $5}' | sort -r | tail -n +4)
for version in $OLD_VERSIONS_FI1; do
  if [ $version -gt 250101 ]; then
    echo "  Deleting fi1:/var/www/html/maps/$version/"
    rclone purge -v fi1:/var/www/html/maps/$version/
  fi
done

# de1 - keep max 6 versions
echo "Cleaning de1 (keeping 6 newest versions)..."
OLD_VERSIONS_DE1=$(rclone lsd de1:/var/www/html/comaps-cdn/maps --max-depth 1 | awk '{print $5}' | sort -r | tail -n +7)
for version in $OLD_VERSIONS_DE1; do
  if [ $version -gt 250101 ]; then
    echo "  Deleting de1:/var/www/html/comaps-cdn/maps/$version/"
    rclone purge -v de1:/var/www/html/comaps-cdn/maps/$version/
  fi
done

# fr1 - keep max 6 versions
echo "Cleaning fr1 (keeping 6 newest versions)..."
OLD_VERSIONS_FR1=$(rclone lsd fr1:/data/maps --max-depth 1 | awk '{print $5}' | sort -r | tail -n +7)
for version in $OLD_VERSIONS_FR1; do
  if [ $version -gt 250101 ]; then
    echo "  Deleting fr1:/data/maps/$version/"
    rclone purge -v fr1:/data/maps/$version/
  fi
done

# us2 - keep all versions (no cleanup)
echo "Skipping us2 cleanup (keeping all versions)"

echo "Old version cleanup complete"

echo "Uploading to us2"
# An explicit mwm/txt filter is used to skip temp files when run for an unfinished generation
rclone copy -v --include "*.{mwm,txt}" $DIR us2:comaps-map-files/maps/$MAPS &

echo "Uploading to ru1"
rclone copy -v --include "*.{mwm,txt}" $DIR ru1:comaps-maps/maps/$MAPS &

echo "Uploading to fi1"
rclone copy -v --include "*.{mwm,txt}" $DIR fi1:/var/www/html/maps/$MAPS &

echo "Uploading to de1"
rclone copy -v --include "*.{mwm,txt}" $DIR de1:/var/www/html/comaps-cdn/maps/$MAPS &

echo "Uploading to fr1"
rclone copy -v --include "*.{mwm,txt}" $DIR fr1:/data/maps/$MAPS &

# us1 is not used for maps atm
# rclone lsd us1:/home/dh_zzxxrk/cdn-us-1.comaps.app/maps

wait

echo "Running once more without parallelization to output status:"

echo "us2 status:"
rclone copy -v --include "*.{mwm,txt}" $DIR us2:comaps-map-files/maps/$MAPS

echo "ru1 status:"
rclone copy -v --include "*.{mwm,txt}" $DIR ru1:comaps-maps/maps/$MAPS

echo "fi1 status:"
rclone copy -v --include "*.{mwm,txt}" $DIR fi1:/var/www/html/maps/$MAPS

echo "de1 status:"
rclone copy -v --include "*.{mwm,txt}" $DIR de1:/var/www/html/comaps-cdn/maps/$MAPS

echo "fr1 status:"
rclone copy -v --include "*.{mwm,txt}" $DIR fr1:/data/maps/$MAPS

echo "Upload complete"
