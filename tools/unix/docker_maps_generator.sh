#!/usr/bin/env bash

#Volumes/paths for downloads:
#home/planet/planet/planet.o5m
#home/planet/planet/planet.o5m.md5
#PLANET_COASTS_URL:file:///home/planet/planet/
  #home/planet/planet/latest_coasts.geom and latest_coasts.rawgeom
#SUBWAY_URL: file:///home/planet/subway/beta.json
  #home/planet/subway/beta.json
#HOTELS_URL:/home/planet/planet/kayak/
  #home/planet/planet/kayak/
#SRTM_PATH:/home/planet/SRTM-patched-europe/
#ISOLINES_PATH:/home/planet/planet/isolines/
#ADDRESSES_PATH:/home/planet/planet/tiger/
#UK_POSTCODES_URL:/home/planet/postcodes/gb-postcode-data/gb_postcodes.csv
#US_POSTCODES_URL:/home/planet/postcodes/us-postcodes/uszips.csv

echo "<$(date +%T)> Starting..."

# Prepare paths
#
# Already created by Dockerfile:
#   /root/OM
#   /root/OM/organicmaps
#   /root/OM/maps_build
#   /home/planet
#
mkdir /root/.config
mkdir /root/.config/OMaps # Odd mkdir permission errors in generator_tool in Docker without these
chmod 777 /root/.config
chmod 777 /root/.config/OMaps
mkdir ~/OM/maps_build
mkdir ~/OM/omim-build-release
mkdir ~/OM/osmctools
mkdir /home/planet/planet
mkdir /home/planet/planet/isolines/
mkdir /home/planet/planet/kayak
mkdir /home/planet/planet/tiger/
mkdir /home/planet/postcodes/
mkdir /home/planet/postcodes/gb-postcode-data/
mkdir /home/planet/postcodes/us-postcodes/
mkdir /home/planet/SRTM-patched-europe/
mkdir /home/planet/subway

echo "Writing S3 config..."
echo "[default]" > ~/.s3cfg
echo "access_key = $S3_KEY_ID" >> ~/.s3cfg
echo "secret_key = $S3_SECRET_KEY" >> ~/.s3cfg
echo "host_base = $S3_HOST_BASE" >> ~/.s3cfg
echo "host_bucket = $(bucket)s.$S3_HOST_BASE" >> ~/.s3cfg
# S3_BUCKET is used during upload

echo "Wrote:"
cat ~/.s3cfg

echo "<$(date +%T)> Compiling tools..."
cd ~/OM/organicmaps
./tools/unix/build_omim.sh -r generator_tool
./tools/unix/build_omim.sh -r world_roads_builder_tool
./tools/unix/build_omim.sh -r mwm_diff_tool
cd tools/python/maps_generator
python3 -m venv /tmp/venv
/tmp/venv/bin/pip3 install -r requirements_dev.txt

echo "<$(date +%T)> Copying map generator INI..."
cp var/etc/map_generator.ini.prod var/etc/map_generator.ini

#TODO: may be duplicated by maps_generator at "osmctools are not found, building from the sources"
echo "<$(date +%T)> Prebuild some tools so we can make an o5m file or run update_planet..."
cd ~/OM/organicmaps/tools/osmctools
gcc osmupdate.c -l z -o ~/OM/osmctools/osmupdate
gcc osmconvert.c -l z -o ~/OM/osmctools/osmconvert

# May be unnecessary when running world
# /tmp/venv/bin/python -m maps_generator --coasts
# save to /path/to/coasts WorldCoasts.geom as latest_coasts.geom and WorldCoasts.rawgeom latest_coasts.rawgeom
# (from https://github.com/mapsme/omim/issues/11994)

cd /home/planet/planet
if [ ! -f planet-latest.osm.pbf ]; then
  echo "<$(date +%T)> Downloading planet-latest.osm.pbf..."
  # curl -OL https://ftpmirror.your.org/pub/openstreetmap/pbf/planet-latest.osm.pbf
  echo "<$(date +%T)> Downloading planet-latest.osm.pbf.md5..."
  curl -OL https://ftpmirror.your.org/pub/openstreetmap/pbf/planet-latest.osm.pbf.md5
else
  echo "<$(date +%T)> planet-latest.osm.pbf exists, not downloading..."
fi

#curl -OL https://download.geofabrik.de/north-america/us-west-latest.osm.pbf
#curl -OL https://download.geofabrik.de/north-america/us-west-latest.osm.pbf.md5
# (rename us-west-latest to planet-latest and edit the md5 file accordingly)
if [ ! -f planet.o5m ]; then
  echo "<$(date +%T)> Converting planet-latest.osm.pbf to planet.o5m..."
  ~/OM/osmctools/osmconvert planet-latest.osm.pbf -o=planet.o5m
else
  echo "<$(date +%T)> planet.o5m exists, not converting..."
fi
# (currently unused:) ~/OM/organicmaps/tools/unix/update_planet.sh planet.o5m

echo "<$(date +%T)> Generating maps..."
cd ~/OM/organicmaps/tools/python
/tmp/venv/bin/python -m maps_generator --production
#/tmp/venv/bin/python -m maps_generator --countries="World, WorldCoasts, US_Oregon_*, US_California_*, US_Washington_*" --production
#/tmp/venv/bin/python -m maps_generator --countries="US_Oregon_Portland" --skip="Coastline"

echo "<$(date +%T)> Uploading maps..."
# maps.zyphon.com:www <<EOF
# Needs StrictHostKeyChecking=no otherwise new containers/SFTP_HOSTs will require a manual ssh attempt
#sshpass -p $SFTP_PASSWORD sftp -o StrictHostKeyChecking=no $SFTP_USER@$SFTP_HOST:$SFTP_PATH <<EOF
#put ~/OM/maps_build/generation.log
#put ~/OM/maps_build/20*/2*/*.mwm
#put ~/OM/maps_build/20*/logs
#exit
#EOF

s3cmd put ~/OM/maps_build/generation.log "s3://$S3_BUCKET/$(date +%y%m%d)/"
s3cmd put ~/OM/maps_build/*/*/*.mwm "s3://$S3_BUCKET/$(date +%y%m%d)/" --recursive
s3cmd put ~/OM/maps_build/*/logs "s3://$S3_BUCKET/$(date +%y%m%d)/" --recursive

echo "<$(date +%T)> Removing intermediate data..."
rm -rf ~/OM/maps_build/*/intermediate_data
# rm -rf ~/OM/

echo "<$(date +%T)> DONE"
