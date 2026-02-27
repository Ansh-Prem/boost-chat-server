#pragma once
#include <boost/asio.hpp>
#include "core/RoomManager.hpp"
#include "util/IdGenerator.hpp"

namespace net {
  namespace asio = boost::asio;
  using tcp = asio::ip::tcp;

  class WsServer {
  public:
    WsServer(asio::io_context& io, tcp::endpoint ep, core::RoomManager& rooms, util::IdGenerator& ids);
    void start();

  private:
    void do_accept();

    asio::io_context& io_;
    tcp::acceptor acceptor_;
    core::RoomManager& rooms_;
    util::IdGenerator& ids_;
  };
}