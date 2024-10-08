#include <boost/asio.hpp>
#include <deque>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

using boost::asio::ip::tcp;

// Предварительное объявление класса Server
class Server;

class Session : public std::enable_shared_from_this<Session> {
 public:
  Session(tcp::socket socket, Server& server);
  void start();
  void deliver(const std::vector<char>& msg);

 private:
  void do_read();
  void do_write();

  tcp::socket socket_;
  Server& server_;
  std::array<char, 1024> read_msg_;
  std::deque<std::vector<char>> write_msgs_;
};

class Server {
 public:
  Server(boost::asio::io_context& io_context, short port);
  void deliver(const std::vector<char>& msg);
  void join(std::shared_ptr<Session> session);
  void leave(std::shared_ptr<Session> session);

 private:
  void do_accept();

  tcp::acceptor acceptor_;
  std::set<std::shared_ptr<Session>> participants_;
};

// Реализация методов Session
Session::Session(tcp::socket socket, Server& server)
    : socket_(std::move(socket)), server_(server) {}

void Session::start() { do_read(); }

void Session::deliver(const std::vector<char>& msg) {
  bool write_in_progress = !write_msgs_.empty();
  write_msgs_.push_back(msg);
  if (!write_in_progress) {
    do_write();
  }
}

void Session::do_read() {
  auto self(shared_from_this());
  socket_.async_read_some(
      boost::asio::buffer(read_msg_),
      [this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
          std::vector<char> msg(read_msg_.begin(), read_msg_.begin() + length);
          server_.deliver(msg);
          do_read();
        } else if (ec != boost::asio::error::operation_aborted) {
          server_.leave(shared_from_this());
        }
      });
}

void Session::do_write() {
  auto self(shared_from_this());
  boost::asio::async_write(
      socket_,
      boost::asio::buffer(write_msgs_.front().data(),
                          write_msgs_.front().size()),
      [this, self](boost::system::error_code ec, std::size_t /*length*/) {
        if (!ec) {
          write_msgs_.pop_front();
          if (!write_msgs_.empty()) {
            do_write();
          }
        } else {
          server_.leave(shared_from_this());
        }
      });
}

// Реализация методов Server
Server::Server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
  do_accept();
}

void Server::deliver(const std::vector<char>& msg) {
  for (auto& participant : participants_) {
    participant->deliver(msg);
  }
}

void Server::join(std::shared_ptr<Session> session) {
  participants_.insert(session);
}

void Server::leave(std::shared_ptr<Session> session) {
  participants_.erase(session);
}

void Server::do_accept() {
  acceptor_.async_accept(
      [this](boost::system::error_code ec, tcp::socket socket) {
        if (!ec) {
          std::cout << "New connection" << std::endl;
          auto session = std::make_shared<Session>(std::move(socket), *this);
          join(session);
          session->start();
        }
        do_accept();
      });
}

int main() {
  try {
    boost::asio::io_context io_context;
    Server server(io_context, 8080);
    std::cout << "Server running on port 8080" << std::endl;
    io_context.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
  return 0;
}
