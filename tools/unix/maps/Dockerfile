# See run-docker.sh for cloning, building, and running the maps generator Docker routine
FROM debian:latest

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

RUN apt-get update -qq \
    && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cargo \
    clang \
    cmake \
    curl \
    git \
    jq \
    libc++-dev \
    libfreetype-dev \
    libgeos-dev \
    libgl1-mesa-dev \
    libglvnd-dev \
    libharfbuzz-dev \
    libicu-dev \
    libqt6positioning6 \
    libqt6positioning6-plugins \
    libqt6svg6-dev \
    libsqlite3-dev \
    libxcursor-dev \
    libxi-dev \
    libxinerama-dev \
    libxrandr-dev \
    ninja-build \
    nodejs \
    openssh-client \
    osmctools \
    osmium-tool \
    pyosmium \
    python3 \
    python3-pip \
    python3-venv \
    python-is-python3 \
    python3-dev \
    qt6-base-dev \
    qt6-positioning-dev \
    rclone \
    rustc \
    sshpass \
    vim-tiny \
    wget \
    wget2 \
    zlib1g-dev \
    && rm -rf /var/cache/apt/* /var/lib/apt/lists/*;

# Allow pip to install system-wide in the container
RUN pip3 install "protobuf<4" --break-system-packages

# Mount a volume to store the large input, output, and temp files here
RUN mkdir /mnt/4tbexternal
# And a volume to store >10gb files for the planet output here
RUN mkdir /home/planet

WORKDIR ~

# The actual map generation process will run something like this,
# but ~/comaps isn't mounted nor provided in the docker image.
#CMD ~/comaps/tools/unix/maps/docker_maps_generator.sh

CMD /bin/bash