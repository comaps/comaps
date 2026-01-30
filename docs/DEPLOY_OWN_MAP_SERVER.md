# Deploy your maps files server

This doc explains how to deploy your own instance of a CoMaps server with files from official CDNs (We are working to be able to download maps files without hardcoded countries.txt file embedded in the app)
We explain how to deploy with minimal config, but each tool has different options to change server port or choose maps files that you want to download.

## Deploy a server
Our community has developed different tools to deploy easily an instance of a CoMaps server:
- [comaps-map-distributor](https://codeberg.org/gedankenstuecke/comaps-map-distributor)
- [comaps-server](https://github.com/myanesp/comaps-server)

### Deploy comaps-map-distributor

Prerequisites
- [python3](https://www.python.org/downloads/) and [pip](https://pypi.org/project/pip/)
- Your server must be accessible from your network

- Launch your terminal
- Run `pip install comaps-map-distributor`
- Launch the tool with this command `comaps-map-distributor download-maps`
- Choose map files you want to download from official CDNs
- Run `comaps-map-distributor serve-maps`
- Go to your mobile device -> CoMaps -> settings -> Advanced -> Custom Maps server
- Edit URL with your URL server and enjoy

### Deploy comaps-server

Prerequisites
- Docker
- Your server must be accessible from your network

#### Docker

- Launch your terminal
- Run ``` docker run -d \
  --name comaps-server \
  --restart unless-stopped \
  -e MAPS=all \ 
  -e OUTPUT_DIR=/maps \
  -p "80:80" \
  ghcr.io/myanesp/comaps-server:latest```
- Go to your mobile device -> CoMaps -> settings -> Advanced -> Custom Maps server
- Edit URL with your URL server and enjoy   

#### Docker compose
- Launch your terminal
- Create a `compose.yml` file with this config and save it:

```services:
  maps-server:
    image: ghcr.io/myanesp/comaps-server
    container_name: comaps-server
    ports:
      - "80:80"
    environment:
      - MAPS=World,WorldCoasts,Spain
      - OUTPUT_DIR=/maps
    volumes:
      - ./maps:/maps
      - TZ=Europe/Madrid```

- Execute `docker compose up`	  
- Go to your mobile device -> CoMaps -> settings -> Advanced -> Custom Maps server
- Edit URL with your URL server and enjoy   

You can find more details in the [FAQ article](https://www.comaps.app/support/how-can-i-host-a-custom-map-server-for-downloads/) to deploy your own HTTP maps server and find more details [here](https://www.comaps.app/support/how-can-i-set-a-custom-map-server-for-downloads/) about restrictions.
