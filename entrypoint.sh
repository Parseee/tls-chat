#!/bin/sh

# Generate self-signed TLS cert if it does not exist
if [ ! -f cert.pem ] || [ ! -f key.pem ]; then
    echo "TLS certificates missing. Generating self-signed keypair..."
    openssl req -x509 -newkey rsa:4096 \
        -keyout key.pem \
        -out cert.pem \
        -sha256 -days 365 -nodes \
        -subj "/CN=localhost"
fi

# Hand off execution to the compiled C++ application binary
exec "$@"