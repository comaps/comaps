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

echo "<$(date +%T)> Copying map generator INI..."
cd ~/comaps/tools/python/maps_generator
cp var/etc/map_generator.ini.prod var/etc/map_generator.ini

GENARGS=()
SKIP="MwmDiffs"

if [ $MWMTEST == "true" ]; then
    echo "Marking as a test (non-prod) generation"
    # TODO: output test maps into e.g. osm-maps-test/ and use a different generation.log
    GENARGS+=(-s="test")
fi

if [ $MWMCONTINUE == "true" ]; then
    echo "Continuing from preexisting generator run"
    GENARGS+=(--continue)
fi

if [[ -n $MWMCOUNTRIES ]]; then
    echo "Generating only specific maps for [$MWMCOUNTRIES]"
    GENARGS+=(--countries="$MWMCOUNTRIES")
    SKIP="$SKIP,Coastline"
fi

GENARGS+=(--skip="$SKIP")
cd ~/comaps/tools/python
echo "<$(date +%T)> Generating maps (extra args: ${GENARGS[@]})..."
/tmp/venv/bin/python -m maps_generator "${GENARGS[@]}"

echo "<$(date +%T)> DONE"
