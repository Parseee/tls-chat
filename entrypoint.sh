#!/bin/sh

HOST_CERT="/etc/letsencrypt/cert.pem"
HOST_KEY="/etc/letsencrypt/key.pem"


if [ -f "$HOST_CERT" ] && [ -f "$HOST_KEY" ]; then
    echo "Production certificates detected. Binding to application layer..."
    ln -sf "$HOST_CERT" cert.pem
    ln -sf "$HOST_KEY" key.pem
else
    echo "Required production keys missing. Generating temporary fallback credentials..."
    openssl req -x509 -newkey rsa:4096 \
        -keyout key.pem \
        -out cert.pem \
        -sha256 -days 365 -nodes \
        -subj "/CN=localhost"
fi

exec "$@"
