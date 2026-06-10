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
  Connection(ssl::stream<tcp::socket> ws_stream, ChatServer &server)
      : ws_(std::move(ws_stream)), server_(server) {}
  void run(http::request<http::string_body> req);
  void send(std::shared_ptr<std::string> msg_ptr) {
    write_queue_.push(msg_ptr);

    if (write_queue_.size() == 1) {
      ws_.text(true);
      do_write();
    }
  }

private:
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
        : stream_(std::move(socket), ctx), server_(server) {}

    void run() {
      auto self = shared_from_this();
      stream_.async_handshake(ssl::stream_base::server,
                              [this, self](boost::beast::error_code err) {
                                if (!err) {
                                  do_read();
                                }
                              });
    }

  private:
    void do_read() {
      auto self = shared_from_this();
      http::async_read(
          stream_, buffer_, req_,
          [this, self](boost::beast::error_code err, std::size_t) {
            if (err) {
              return;
            }

            boost::beast::string_view target = req_.target();

            if (websocket::is_upgrade(req_) && target == "/ws") {
              auto session =
                  std::make_shared<Connection>(std::move(stream_), server_);
              server_.join(session);
              session->run(std::move(req_));
            } else if (target == "/" || target == "/index.html") {
              send_file("static/index.html");
            } else if (target == "/docs" || target == "/docs.html") {
              send_file("static/docs.html");
            } else {
              send_not_found();
            }
          });
    }

    void send_file(std::string const &filepath) {
      auto self = shared_from_this();
      auto res = std::make_shared<http::response<http::file_body>>();
      res->version(req_.version());
      res->result(http::status::ok);
      res->set(http::field::server, "tls-chat");
      res->set(http::field::content_type, "text/html; charset=utf-8");

      boost::beast::error_code err;
      res->body().open(filepath.c_str(), boost::beast::file_mode::scan, err);
      if (err) {
        send_not_found();
        return;
      }
      res->prepare_payload();

      http::async_write(
          stream_, *res,
          [this, self, res](boost::beast::error_code err, std::size_t) {
            stream_.async_shutdown([self](boost::beast::error_code) {});
          });
    }

    void send_not_found() {
      auto self = shared_from_this();
      auto res = std::make_shared<http::response<http::string_body>>(
          http::status::not_found, req_.version());
      res->set(http::field::server, "tls-chat");
      res->set(http::field::content_type, "text/plain");
      res->body() = "Not found";
      res->prepare_payload();

      http::async_write(
          stream_, *res,
          [this, self, res](boost::beast::error_code err, std::size_t) {
            stream_.async_shutdown([self](boost::beast::error_code) {});
          });
    }

    ssl::stream<tcp::socket> stream_;
    ChatServer &server_;
    boost::beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
  };

  asio::io_context &ioctx_;
  ssl::context &sslctx_;
  tcp::acceptor acceptor_;
  std::set<std::shared_ptr<Connection>> registry_;
};

void Connection::run(http::request<http::string_body> req) {
  auto self = shared_from_this();
  ws_.async_accept(req, [this, self](boost::beast::error_code err) {
    if (!err) {
      do_read();
    } else {
      std::cerr << "wc_accept error: " << err.message() << std::endl;
      server_.leave(shared_from_this());
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
          
          if (msg.find("/ip") != std::string::npos) {
            try {
              std::string ip = ws_.next_layer().next_layer().remote_endpoint().address().to_string();
              msg = "[IP: " + ip + "] " + msg;
            } catch (const std::exception&) {
            }
          }

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