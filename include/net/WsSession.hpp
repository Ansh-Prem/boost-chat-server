#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <deque>
#include <memory>
#include <string>

#include "core/RoomManager.hpp"
#include "core/CommandParser.hpp"

namespace net {
  namespace asio = boost::asio;
  namespace beast = boost::beast;
  namespace http = beast::http;
  namespace websocket = beast::websocket;
  using tcp = asio::ip::tcp;

  class WsSession : public std::enable_shared_from_this<WsSession> {
  public:
    WsSession(tcp::socket socket, core::RoomManager& rooms, std::uint64_t id);

    void start();

  private:
    void on_accept(beast::error_code ec);

    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes);

    void deliver(const std::string& msg);
    void do_write();

    void join_room(const std::string& room);
    std::string display_name() const;

    websocket::stream<tcp::socket> ws_;

    // ✅ changed from strand<any_io_executor>
    asio::strand<asio::io_context::executor_type> strand_;

    beast::flat_buffer buffer_;

    core::RoomManager& rooms_;
    std::shared_ptr<core::ChatRoom> room_;
    std::uint64_t id_{};
    std::string nick_;

    std::deque<std::string> outq_;
    static constexpr std::size_t MAX_QUEUE = 256;
  };
}