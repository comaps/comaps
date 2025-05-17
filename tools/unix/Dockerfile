# See run-docker.sh for cloning, building, and running the maps generator Docker routine
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
