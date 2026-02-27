#pragma once
#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <string>

#include "core/RoomManager.hpp"
#include "core/CommandParser.hpp"
#include "util/Log.hpp"

namespace net {
  namespace asio = boost::asio;
  using tcp = asio::ip::tcp;

  class TcpSession : public std::enable_shared_from_this<TcpSession> {
  public:
    TcpSession(tcp::socket socket, core::RoomManager& rooms, std::uint64_t id);

    void start();
    void deliver(const std::string& msg);

  private:
    void do_read();
    void on_line(const std::string& line);
    void do_write();

    void join_room(const std::string& room);
    std::string display_name() const;

    tcp::socket socket_;

    // ✅ changed from strand<any_io_executor>
    asio::strand<asio::io_context::executor_type> strand_;

    core::RoomManager& rooms_;
    std::shared_ptr<core::ChatRoom> room_;

    std::uint64_t id_{};
    std::string nick_{"user"};

    boost::asio::streambuf buf_;
    std::deque<std::string> outq_;

    static constexpr std::size_t MAX_QUEUE = 256;
  };
}