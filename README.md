# Secure Multiplexed WebSocket Chat Server

A secure chat server written in C++20. It multiplexes HTTPS and Secure WebSockets (WSS) over a single port. It is purely web-socket based and does not use any database to store conversations.

## Features

- **TLS-First Multiplexing**: Utilizes Boost.Asio, Boost.Beast, and OpenSSL to handle initial TLS handshakes before protocol demultiplexing.
- **HTTP Static Hosting**: Serves the frontend web client (`static/index.html`) on standard HTTPS requests to `/`.
- **WebSocket Upgrades**: Routes requests to `/ws` into a persistent, stateful WebSocket connection for real-time, full-duplex messaging.
- **Broadcast Architecture**: Messages sent from one client are asynchronously and safely broadcasted to all other active connections. New users will recieve only new messages.
- **Container-Ready**: Features a multi-stage Docker build tailored for both Devcontainer development and production-like deployment.

## Prerequisites

- Docker & Docker Compose
- OpenSSL (to generate local TLS certificates)

## Setup & Execution

### 1. Build and Run

Use the provided Docker Compose configuration to build the image and start the service in the background:

```bash
docker compose -f .devcontainer/compose.yml up -d --build
```

### 2. Usage

1. Open your web browser and navigate to the desired web page (in my case i have hardcoded it into my domain).
2. Since you are using a self-signed certificate, bypass the browser's security warning.
3. The server will serve the chat interface. Type a message and hit **Send** to broadcast it to all connected clients!

## Tech Stack

- **Language:** C++20
- **Libraries:** Boost.Asio, Boost.Beast, OpenSSL
- **Infrastructure:** Docker, CMake
