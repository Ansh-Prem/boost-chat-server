#include "net/WsServer.hpp"
#include "net/WsSession.hpp"
#include "util/Log.hpp"

namespace net {

WsServer::WsServer(asio::io_context& io, tcp::endpoint ep, core::RoomManager& rooms, util::IdGenerator& ids)
  : io_(io), acceptor_(io), rooms_(rooms), ids_(ids) {
  boost::system::error_code ec;
  acceptor_.open(ep.protocol(), ec);
  acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
  acceptor_.bind(ep, ec);
  acceptor_.listen(asio::socket_base::max_listen_connections, ec);
}

void WsServer::start() {
  util::info("WS server listening on " + acceptor_.local_endpoint().address().to_string() + ":" + std::to_string(acceptor_.local_endpoint().port()));
  do_accept();
}

void WsServer::do_accept() {
  acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
    if (!ec) {
      auto id = ids_.next();
      std::make_shared<WsSession>(std::move(socket), rooms_, id)->start();
    }
    do_accept();
  });
}

}