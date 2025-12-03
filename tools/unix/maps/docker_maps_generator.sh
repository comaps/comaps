#!/usr/bin/env bash

set -e

echo "<$(date +%T)> Starting..."

# Prepare paths
# Most other paths in /mnt/4tbexternal or /home/planet are already created by Dockerfile or CI/CD.
#
mkdir -p /root/.config/CoMaps # Odd mkdir permission errors in generator_tool in Docker without these
chmod -R 777 /root/.config
mkdir -p /home/planet/postcodes/gb-postcode-data/
mkdir -p /home/planet/postcodes/us-postcodes/
mkdir -p /home/planet/SRTM-patched-europe/
mkdir -p /home/planet/subway

echo "<$(date +%T)> Running ./configure.sh ..."
cd ~/comaps
export SKIP_MAP_DOWNLOAD=1 SKIP_GENERATE_SYMBOLS=1
./configure.sh

echo "<$(date +%T)> Compiling tools..."
cd ~/comaps
./tools/unix/build_omim.sh -p ~ -R generator_tool
./tools/unix/build_omim.sh -p ~ -R world_roads_builder_tool
./tools/unix/build_omim.sh -p ~ -R mwm_diff_tool
cd tools/python/maps_generator
python3 -m venv /tmp/venv
/tmp/venv/bin/pip3 install -r requirements_dev.txt

echo "<$(date +%T)> Copying map generator INI..."
cp var/etc/map_generator.ini.prod var/etc/map_generator.ini

GENARGS=""

if [ $MWMTEST -gt 0 ]; then
    echo "Marking as a test (non-prod) generation"
    # TODO: output test maps into e.g. osm-maps-test/ and use a different generation.log
    GENARGS="$GENARGS -s=test"
fi

if [ $MWMCONTINUE -gt 0 ]; then
    echo "Continuing from preexisting generator run"
    GENARGS="$GENARGS --continue"
fi

if [[ -n $MWMCOUNTRIES ]]; then
    echo "Generating only specific maps for [$MWMCOUNTRIES]"
    GENARGS="$GENARGS --countries=$MWMCOUNTRIES"
fi

cd ~/comaps/tools/python
echo "<$(date +%T)> Generating maps (extra args: $GENARGS)..."
/tmp/venv/bin/python -m maps_generator --skip="MwmDiffs" $GENARGS

echo "<$(date +%T)> DONE"
