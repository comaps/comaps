# Check out this whole repo with submodules and shallow:

# cd /path/to/4tb
# git clone --recurse-submodules --shallow-submodules https://codeberg.org/comaps/comaps.git
# cd /path/to/4tb/comaps
# git checkout docker_maps_generator

#TODO: shaders_complier ?

# Get static data:
# cd /path/to/4tb/comaps/data/
# wget World.mwm
# wget WorldCoasts.mwm
#TODO: isolines, postcodes, subways, wiki

# Build with: docker build . -t maps_generator
# Edit as appropriate and run with:
#  docker run \
#  -e S3_KEY_ID=foo -e S3_SECRET_KEY=bar -e S3_HOST_BASE=digitaloceanspaces.com -e S3_BUCKET=comaps-map-files \
#  -v /path/to/comaps:/root/OM/organicmaps -v /path/to/4tb/osm-planet:/home/planet \
#  -v /path/to/4tb/osm-maps:/root/OM/maps_build -it maps_generator

FROM ubuntu:noble

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC 

RUN apt-get update -qq \
    && apt-get install -y --no-install-recommends \
    curl \
    osmctools \
    s3cmd \
    sshpass \
    vim \
    wget \
    build-essential \
    clang \
    cmake \
    ninja-build \
    python3 \
    python3-pip \
    python3.12-venv \
    qt6-base-dev \
    qt6-positioning-dev \
    libc++-dev \
    libfreetype-dev \
    libglvnd-dev \
    libgl1-mesa-dev \
    libharfbuzz-dev \
    libicu-dev \
    libqt6svg6-dev \
    libqt6positioning6-plugins \
    libqt6positioning6 \
    libsqlite3-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    zlib1g-dev \
    && rm -rf /var/cache/apt/* /var/lib/apt/lists/*;

RUN mkdir /root/OM
# When running, bind mount the repo here
RUN mkdir /root/OM/organicmaps
# And a volume to store the large output/temp files here
RUN mkdir /root/OM/maps_build
# And a volume to store >10gb files for the planet here
RUN mkdir /home/planet

WORKDIR /root/OM/organicmaps

# For debugging
#CMD /bin/bash 

CMD /root/OM/organicmaps/tools/unix/docker_maps_generator.sh
