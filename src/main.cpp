#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <iostream>
#include <memory>
#include <queue>
#include <set>

constexpr uint32_t kDefaultPort = 8888;

namespace http = boost::beast::http;
namespace websocket = boost::beast::websocket;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
using tcp = boost::asio::ip::tcp;

class ChatServer;

class Connection : public std::enable_shared_from_this<Connection> {
public:
  Connection(tcp::socket &&socket, ssl::context &ssl_ctx, ChatServer &server)
      : ws_(std::move(socket), ssl_ctx), server_(server) {}
  void run();
  void send(std::shared_ptr<std::string> msg_ptr) {
    write_queue_.push(msg_ptr);

    if (write_queue_.size() == 1) {
      do_write();
    }
  }

private:
  void do_ws_accept();
  void do_read();
  void do_write();

  websocket::stream<ssl::stream<tcp::socket>> ws_;
  boost::beast::flat_buffer buf_;
  ChatServer &server_;
  std::queue<std::shared_ptr<std::string>> write_queue_;
};

class ChatServer {
public:
  ChatServer(asio::io_context &ioctx, ssl::context &sslctx,
             tcp::endpoint endpoint)
      : ioctx_(ioctx), sslctx_(sslctx), acceptor_(ioctx, endpoint) {
    do_accept();
  }

  void join(std::shared_ptr<Connection> session) { registry_.insert(session); }
  void leave(std::shared_ptr<Connection> session) { registry_.erase(session); }

  void broadcast(const std::string &msg, std::shared_ptr<Connection> conn) {
    auto msg_ptr = std::make_shared<std::string>(std::move(msg));

    for (auto const &session : registry_) {
      if (session != conn) {
        session->send(msg_ptr);
      }
    }
  }

private:
  void do_accept() {
    acceptor_.async_accept(
        [this](boost::beast::error_code err, tcp::socket socket) {
          if (!err) {
            std::make_shared<HttpDetector>(std::move(socket), sslctx_, *this)
                ->run();
          }
          do_accept();
        });
  }

  class HttpDetector : public std::enable_shared_from_this<HttpDetector> {
  public:
    HttpDetector(tcp::socket socket, ssl::context &ctx, ChatServer &server)
        : socket_(std::move(socket)), ctx_(ctx), server_(server) {}

    void run() {
      auto self = shared_from_this();
      do_read();
    }

  private:
    void do_read() {
      auto session =
          std::make_shared<Connection>(std::move(socket_), ctx_, server_);
      server_.join(session);

      session->run();
    }

    tcp::socket socket_;
    ssl::context &ctx_;
    ChatServer &server_;
    boost::beast::flat_buffer buffer_;
  };

  asio::io_context &ioctx_;
  ssl::context &sslctx_;
  tcp::acceptor acceptor_;
  std::set<std::shared_ptr<Connection>> registry_;
};

void Connection::run() {
  auto self = shared_from_this();
  ws_.next_layer().async_handshake(
      ssl::stream_base::server, [this, self](boost::beast::error_code err) {
        if (!err) {
          do_ws_accept();
        } else {
          std::cerr << "TLS Handshake failed: " << err.message() << std::endl;
          server_.leave(shared_from_this());
        }
      });
}

void Connection::do_ws_accept() {
  auto self = shared_from_this();
  ws_.async_accept([this, self](boost::beast::error_code err) {
    if (!err) {
      do_read();
    } else {
      std::cerr << "wc_accept error: " << err.message() << std::endl;
    }
  });
}

void Connection::do_read() {
  auto self = shared_from_this();
  std::cout << "connection read" << std::endl;
  ws_.async_read(
      buf_, [this, self](boost::beast::error_code err, size_t bytes_trasfered) {
        if (!err) {
          std::string msg = boost::beast::buffers_to_string(buf_.data());
          server_.broadcast(msg, self);
          buf_.consume(bytes_trasfered);
          do_read();
        } else {
          std::cerr << "read error: " << err.message() << std::endl;
        }
      });
}

void Connection::do_write() {
  auto self = shared_from_this();

  ws_.async_write(
      asio::buffer(*write_queue_.front()),
      [this, self](boost::beast::error_code err, size_t bytes_written) {
        if (!err) {
          write_queue_.pop();

          if (!write_queue_.empty()) {
            do_write();
          }
        } else {
          server_.leave(shared_from_this());
        }
      });
}

int main() {
  try {
    asio::io_context ioc;
    ssl::context ctx{ssl::context::tlsv13_server};

    ctx.use_certificate_chain_file("cert.pem");
    ctx.use_private_key_file("key.pem", ssl::context::pem);

    tcp::endpoint endpoint{asio::ip::make_address("0.0.0.0"), kDefaultPort};
    ChatServer server(ioc, ctx, endpoint);

    std::cout << "Secure WebSocket Server acting on port " << kDefaultPort
              << "...\n";
    ioc.run();
  } catch (std::exception const &e) {
    std::cerr << "Fatal Error: " << e.what() << "\n";
  }
}