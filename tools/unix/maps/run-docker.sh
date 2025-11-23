#!/usr/bin/env bash

# Run the maps generator via Docker manually without CI
# See .forgejo/workflows/map-generator.yml for steps to run before the main mapgen process,
# e.g. clone the repos, get/update planet file, generate isolines etc.

# To build the docker container:
# cd /mnt/4tbexternal/comaps/tools/unix/maps
# docker build . -t maps_generator
#
# To push for ci/cd, tag for codeberg:
#   docker login codeberg.org
#   docker tag maps_generator codeberg.org/comaps/maps_generator:latest
#   docker push codeberg.org/comaps/maps_generator:latest
# You can also tag and push the image Id for posterity: codeberg.org/comaps/maps_generator:1234abcd
# If you get a Dockerfile not found error especially on an XFS partition, try copying Dockerfile to an ext4 partition first.
# Or use docker via apt instead of snap.

# We assume that the following will be cloned into the container itself at runtime:
# ~/comaps (comaps main app repo)
# ~/subways (repo for processing OSM subway/transit info)
# ~/wikiparser (repo for processing Wikipedia data)

# We also assume a number of files/folders/repos are pre-set-up before mounting via volumes below:
# /mnt/4tbexternal (base folder for directory traversal)
# /mnt/4tbexternal/osm-maps (folder for holding generated map data output)
# /home/planet (folder for holding required input dumps)

docker run \
  --ulimit nofile=262144:262144 \
  -v /mnt/4tbexternal/:/mnt/4tbexternal/ \
  -v /mnt/4tbexternal/osm-planet:/home/planet \
  -it codeberg.org/comaps/maps_generator:latest \
  ~/comaps/tools/unix/maps/docker_maps_generator.sh
