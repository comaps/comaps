FROM debian:trixie-slim
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates curl jq openjdk-21-jdk-headless openssl unzip zip \
 && rm -rf /var/lib/apt/lists/*
RUN useradd --uid 10001 --create-home signer
COPY play-signer.sh /play-signer.sh
USER signer
ENTRYPOINT ["bash", "/play-signer.sh"]
