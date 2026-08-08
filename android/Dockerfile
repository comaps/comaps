FROM ubuntu:latest

ENV TZ=Etc/UTC
ENV QT_QPA_PLATFORM=offscreen

RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  cmake \
  qt6-base-dev \
  qt6-svg-dev \
  qt6-positioning-dev \
  libicu-dev \
  libfreetype-dev \
  libharfbuzz-dev \
  libxrandr-dev \
  libxinerama-dev \
  libxcursor-dev \
  libxi-dev \
  optipng \
  python3 \
  python3-venv
WORKDIR /root/comaps
CMD ./gradlew -Parm64 assembleFdroidDebug
