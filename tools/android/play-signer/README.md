# Google Play signer

## Setup

Install Docker and logrotate on the host, copy this directory to `/opt/comaps-play-signer`, then as root:

```sh
cd /opt/comaps-play-signer
mkdir -m 700 secrets
```

Put these in `secrets/`:

- `upload.keystore`
- `google-play.json`
- `upload_key.env`:

```
STORE_PASS=...
KEY_ALIAS=...
KEY_PASS=...
```

```sh
chown -R 10001:10001 secrets
chmod 400 secrets/*
cp comaps-play-signer.cron /etc/cron.d/comaps-play-signer
cp comaps-play-signer.logrotate /etc/logrotate.d/comaps-play-signer
docker compose build
DRY_RUN=1 docker compose run --rm signer
```

## Update or roll back

Copy the files from the wanted commit to `/opt/comaps-play-signer`, then:

```sh
docker compose build
```
