#!/usr/bin/env bash

# Master file documenting how to clone, build, and run the maps generator via Docker

# Prerequisutes:
# sudo apt install docker git

# To bootstrap the repo:
#TODO: change comaps-init to comaps here and throughout
#TODO: change docker_maps_generator to most recent rebase branch
#cd /media/4tbexternal
#if [ ! -f /media/4tbexternal/comaps-init ]; then
#  git clone --recurse-submodules --shallow-submodules https://codeberg.org/comaps/comaps-init.git
#  cd /media/4tbexternal/comaps-init
#  git checkout docker_maps_generator
#else
#  cd /media/4tbexternal/comaps-init
#  git checkout docker_maps_generator
#  git pull origin docker_maps_generator
#fi

# And data:
# cd /media/4tbexternal/comaps-init/data/
# wget World.mwm //pastk - not needed
# wget WorldCoasts.mwm

#TODO: shaders_complier ? //pastk - not needed

#TODO: isolines, postcodes, subways, wiki

#cd /media/4tbexternal/comaps-init/tools/unix
# Build with: docker build . -t maps_generator
# (Good to rebuild each time just in case)
# If you get a Dockerfile not found error especially on an XFS partition, try copying Dockerfile to an ext4 partition
#
# Edit as appropriate and run with:
#  docker run \
#  -e S3_KEY_ID=foo -e S3_SECRET_KEY=bar -e S3_HOST_BASE=digitaloceanspaces.com -e S3_BUCKET=comaps-map-files \
#  --ulimit nofile=262144:262144 \
#  -v /media/4tbexternal/comaps-init:/root/OM/organicmaps -v /media/4tbexternal/osm-planet:/home/planet \
#  -v /media/4tbexternal/osm-maps:/root/OM/maps_build -it maps_generator


docker run -e S3_KEY_ID=changeme -e S3_SECRET_KEY=changeme -e S3_HOST_BASE=change.example.com -e S3_BUCKET=comaps-map-files \
  --ulimit nofile=262144:262144 \
  -v /media/4tbexternal/comaps-init:/root/OM/organicmaps -v /media/4tbexternal/osm-planet:/home/planet \
  -v /media/4tbexternal/osm-maps:/root/OM/maps_build -it maps_generator

