FROM ubuntu:24.04 AS builder
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-asio-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build/build/tls_server .
COPY cert.pem key.pem ./

EXPOSE 8888
CMD ["./tls_server"]