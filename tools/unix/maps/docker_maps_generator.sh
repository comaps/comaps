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


cd ~/comaps/tools/python
if [ $MWMCONTINUE -gt 0 ]; then

echo "<$(date +%T)> Continuing from preexisting generator run..."
/tmp/venv/bin/python -m maps_generator --skip="MwmDiffs" --continue

else

if [[ -n $MWMCOUNTRIES ]]; then

echo "<$(date +%T)> Generating only specific maps [$MWMCOUNTRIES]..."
/tmp/venv/bin/python -m maps_generator --countries=$MWMCOUNTRIES --skip="MwmDiffs"

else

echo "<$(date +%T)> Generating maps..."
/tmp/venv/bin/python -m maps_generator --skip="MwmDiffs"

fi
fi

echo "<$(date +%T)> DONE"
