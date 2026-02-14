# Setting up Forgejo Runners for CoMaps

> This document is a work in progress but should provide enough guidance to eventually figure it out.

- Follow https://forgejo.org/docs/latest/admin/actions/runner-installation/#oci-image-installation to install the Forgejo Runner inside Docker (using docker-compose for the runner, which then itself will run whatever Docker images we want for CI/CD jobs) and register the runner with Codeberg.
- This script may be helpful to get folder structure correct. This document presumes a user ID of 1001. On mapgen we named data `forgejo-runner-data`:

```
#!/usr/bin/env bash

set -e

mkdir -p data
touch data/.runner
mkdir -p data/.cache

chown -R 1001:1001 data/.runner
chown -R 1001:1001 data/.cache
chmod 775 data/.runner
chmod 775 data/.cache
chmod g+s data/.runner
chmod g+s data/.cache
```

- By the end of the instructions (after swapping out the initial sleep `command` for the final forgejo-runner `command`) your docker-compose.yml should look like this. Note the config.yml and volumes.


**for a basic runner like cdn-fi-1:**
```
#version: '3.8'

volumes:
  docker_certs:

services:
  docker:
    image: docker:dind
    hostname: docker
    privileged: 'true'
    volumes:
      - docker_certs:/certs
      - /var/www/html/subways:/var/www/html/subways
      - /tmp/planet:/tmp/planet
    restart: 'always'

  forgejo-runner:
    image: 'data.forgejo.org/forgejo/runner:11.3.0'
    environment:
      DOCKER_HOST: tcp://docker:2376
      DOCKER_TLS_VERIFY: 0
      DOCKER_CERT_PATH: /certs/client
    # User without root privileges, but with access to `./data`.
    user: 1001:1001
    volumes:
      - ./data:/data
      - docker_certs:/certs
      - /var/www/html/subways:/var/www/html/subways
      - /tmp/planet:/tmp/planet
    restart: 'always'

    #command: '/bin/sh -c "while : ; do sleep 1 ; done ;"'
    command: '/bin/sh -c "sleep 5; forgejo-runner daemon -c config.yml"'
```

**for a mapfilemaker server like mapgen-fi-1:** (note the different user ID of 1000, this is an artifact of mapgen-fi-1)

We presume that /mnt/4tbexternal has been created locally.

```
version: '3.8'

services:
  docker-in-docker:
    image: docker:dind
    container_name: 'docker_dind'
    privileged: 'true'
    command: ['dockerd', '-H', 'tcp://0.0.0.0:2375', '--tls=false']
    restart: 'unless-stopped'
    volumes:
      - /mnt/4tbexternal:/media/4tbexternal
      - /mnt/4tbexternal:/mnt/4tbexternal
      - /var/www/html:/var/www/html

  gitea:
    image: 'data.forgejo.org/forgejo/runner:7.0.0'
    links:
      - docker-in-docker
    depends_on:
      docker-in-docker:
        condition: service_started
    container_name: 'runner'
    environment:
      DOCKER_HOST: tcp://docker-in-docker:2375
    # User without root privileges, but with access to `./data`.
    user: 1000:1000
    volumes:
      - ./forgejo-runner-data:/data
      - /mnt/4tbexternal:/media/4tbexternal
      - /mnt/4tbexternal:/mnt/4tbexternal
      - /var/www/html:/var/www/html
    restart: 'unless-stopped'
    command: '/bin/sh -c "sleep 5; forgejo-runner daemon -c config.yml"'
    #command: '/bin/sh -c "while : ; do sleep 1 ; done ;"'
```

- The referenced data/config.yml needs to be created to support long timeouts for mapgen, big capacity on non-mapgen runners, and volume permissions. **For a map gen server, set the capacity to 1 to avoid multiple jobs writing to the same file.**

```
log:
  level: debug
  job_level: debug
runner:
  file: .runner
  capacity: 10
  env_file: .env
  timeout: 3000h
  shutdown_timeout: 30h
container:
  network: bridge
  options: -v /var/run/docker.sock:/var/run/docker.sock
  valid_volumes:
    - '**'
    - /var/run/docker.sock
  docker_host: "-"
```

- Add these labels to the runner by editing `~/forgejo-runners/forgejo-runner-org/data/.runner`:

**cdn-fi-1:**
```
    "comaps-org:host",
    "comaps:host",
    "cdn:host",
    "cdn-fi-1:host",
    "comaps-cdn-fi-1:host",
    "linux:host",
    "comaps-android:docker://codeberg.org/comaps/docker-android-sdk:latest",
    "dind:docker://docker:dind",
    "node20:docker://node:20-bullseye",
    "node-latest:docker://node:latest",
    "ubuntu-22.04:docker://ubuntu:22.04",
    "ubuntu-latest:docker://node:latest"
```

**mapgen-fi-1:**

```
    "mapfilemaker:host",
    " comaps-org:host",
    " comaps:host",
    " comaps-mapgen-fi-1:host",
    " linux:host",
    " dind:host"
```
