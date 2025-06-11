#!/usr/bin/env bash

# Master file documenting how to clone, build, and run the maps generator via Docker

# Prerequisutes:
# sudo apt install docker git

# To bootstrap the repo:
#TODO: rename comaps-init to comaps here and throughout
#cd /media/4tbexternal
#if [ ! -f /media/4tbexternal/comaps-init ]; then
#  git clone --recurse-submodules --shallow-submodules https://codeberg.org/comaps/comaps-init.git
#  cd /media/4tbexternal/comaps-init
#else
#  cd /media/4tbexternal/comaps-init
#  git pull origin
#fi

# And data:
# cd /media/4tbexternal/comaps-init/data/
# wget World.mwm //pastk - not needed
# wget WorldCoasts.mwm

#TODO: isolines, postcodes, subways, wiki

# In tools/unix/maps, copy settings.sh.dist to settings.sh and modify if needed
# In tools/python/maps_generator, copy map_generator.ini.prod to map_generator.ini and modify if needed

#cd /media/4tbexternal/comaps-init/tools/unix
# Build with: docker build . -t maps_generator
# (Good to rebuild each time just in case)
# If you get a Dockerfile not found error especially on an XFS partition, try copying Dockerfile to an ext4 partition first.
#
# Edit as appropriate and run with:
# docker run \
#   -e S3_KEY_ID=changeme \
#   -e S3_SECRET_KEY=changeme \
#   -e S3_ENDPOINT=https://changeme.r2.cloudflarestorage.com/ \
#   -e S3_BUCKET=comaps-map-files \
#   -e SFTP_USER=changeme \
#   -e SFTP_PASSWORD=changeme \
#   -e SFTP_HOST=changeme.dreamhost.com \
#   -e SFTP_PATH=cdn-us-1.comaps.app \
#   --ulimit nofile=262144:262144 \
#   -v /media/4tbexternal/comaps-init:/root/OM/organicmaps \
#   -v /media/4tbexternal/osm-planet:/home/planet \
#   -v /media/4tbexternal/osm-maps:/root/OM/maps_build \
#   -it maps_generator \
#   /root/OM/organicmaps/tools/unix/docker_maps_generator.sh
 
docker run \
  -e S3_KEY_ID=changeme \
  -e S3_SECRET_KEY=changeme \
  -e S3_ENDPOINT=https://changeme.r2.cloudflarestorage.com/ \
  -e S3_BUCKET=comaps-map-files \
  -e SFTP_USER=changeme \
  -e SFTP_PASSWORD=changeme \
  -e SFTP_HOST=changeme.dreamhost.com \
  -e SFTP_PATH=cdn-us-1.comaps.app \
  --ulimit nofile=262144:262144 \
  -v /media/4tbexternal/comaps-init:/root/OM/organicmaps \
  -v /media/4tbexternal/osm-planet:/home/planet \
  -v /media/4tbexternal/osm-maps:/root/OM/maps_build \
  -it maps_generator \
  /root/OM/organicmaps/tools/unix/docker_maps_generator.sh
