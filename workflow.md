### How It Works (Networking Architecture)

1. **Transport Layer (TCP):** Browser initiates standard 3-way handshake with server port 8888.
2. **Security Layer (TLS):** Cryptographic handshake establishes secure symmetric encryption channel. Authenticates server using `cert.pem`. Bypasses eavesdropping.
3. **Application Layer (HTTP Upgrade):** Browser sends HTTP GET request with `Upgrade: websocket` and `Connection: Upgrade` headers. Server processes request, verifies headers, and replies with HTTP status `101 Switching Protocols`.
4. **Persistent Channel (WSS):** Connection protocol shifts instantly to WebSockets over TLS. Connection stays open indefinitely (full-duplex). No HTTP request overhead per message; communication utilizes lightweight frames.
5. **Multiplexing & Broadcast:** Server event loop handles hundreds of connections concurrently on single thread via OS demultiplexer (`epoll` on Linux, `kqueue` on macOS). Connections are stored in central registry (`std::set`). Incoming text frame from one client triggers sequential asynchronous writes to all other registered client sockets.

---

### Project Development Stages

#### Stage 1: Environment Definition

* Initialize workspace repository.
* Configure `devcontainer.json` with `build-essential`, `cmake`, `libboost-asio-dev`, `libssl-dev`.
* Generate self-signed certificate credentials via OpenSSL CLI command.

#### Stage 2: Event Loop & TCP Setup

* Initialize `boost::asio::io_context` execution context.
* Establish `tcp::acceptor` bound to address `0.0.0.0` and port `8888`.
* Implement entry loop to accept incoming raw TCP sockets asynchronously (`async_accept`).

#### Stage 3: Cryptographic Integration

* Instantiate `boost::asio::ssl::context`. Configure it to use TLS server side tokens; load `cert.pem` and `key.pem`.
* Wrap raw sockets into `boost::asio::ssl::stream<tcp::socket>`.
* Invoke asynchronous TLS handshake (`async_handshake`) immediately post-connection.

#### Stage 4: WebSocket Framing (Boost.Beast)

* Layer session protocol: `boost::beast::websocket::stream<boost::asio::ssl::stream<tcp::socket>>`.
* Read initial bytes to parse HTTP structure. If path equals `/ws`, trigger `async_accept` to complete WebSocket handshake protocol negotiation.
* If path equals `/`, reply with HTTP `200 OK` containing raw `index.html` string stream.

#### Stage 5: State Registry & Read Loop

* Design connection manager class using RAII.
* Sessions inherit `std::enable_shared_from_this` to guarantee memory safety during asynchronous callbacks.
* Implement recursive async read loop (`async_read`) on WebSocket stream. Upon packet receipt, iterate registry to call `async_write` on target descriptors. Clean up registry entry immediately on error or disconnection.

#### Stage 6: Containerization & Validation

* Author multi-stage `Dockerfile`.
* **Stage 1 (Builder):** Pull Ubuntu image, install compilers, invoke CMake, output static binary executable.
* **Stage 2 (Runtime):** Pull clean Ubuntu base, extract binary, inject certificates, expose port `8888`.


* Create `compose.yml` defining port mappings and deployment policies. Execute `docker compose up --build`. Run diagnostics via local browser instance.