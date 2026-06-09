FROM ubuntu:24.04 AS builder
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libssl-dev \
    openssl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspaces/tls-chat
COPY --from=builder /build/build/tls-chat .
COPY static ./static/
COPY entrypoint.sh ./
RUN chmod +x entrypoint.sh

EXPOSE 8888
ENTRYPOINT ["./entrypoint.sh"]
CMD ["./tls-chat"]
