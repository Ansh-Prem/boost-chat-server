#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <string>

#include "core/RoomManager.hpp"
#include "net/TcpServer.hpp"
#include "net/WsServer.hpp"
#include "util/IdGenerator.hpp"
#include "util/Log.hpp"

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

struct Args {
  unsigned short tcp_port = 5555;
  unsigned short ws_port  = 8080;
  int threads = 4;
};

static Args parse_args(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    std::string k = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 >= argc) return {};
      return argv[++i];
    };
    if (k == "--tcp") a.tcp_port = static_cast<unsigned short>(std::stoi(next()));
    else if (k == "--ws") a.ws_port = static_cast<unsigned short>(std::stoi(next()));
    else if (k == "--threads") a.threads = std::max(1, std::stoi(next()));
  }
  return a;
}

int main(int argc, char** argv) {
  auto args = parse_args(argc, argv);

  asio::io_context io;
  asio::signal_set signals(io, SIGINT, SIGTERM);
  signals.async_wait([&](auto, auto){ util::warn("Shutdown signal received."); io.stop(); });

  core::RoomManager rooms;
  util::IdGenerator ids;

  net::TcpServer tcp_server(io, tcp::endpoint(tcp::v4(), args.tcp_port), rooms, ids);
  net::WsServer  ws_server (io, tcp::endpoint(tcp::v4(), args.ws_port ), rooms, ids);

  tcp_server.start();
  ws_server.start();

  util::info("Running with threads=" + std::to_string(args.threads));

  std::vector<std::thread> pool;
  pool.reserve(args.threads);
  for (int t = 0; t < args.threads; ++t) {
    pool.emplace_back([&]{ io.run(); });
  }

  for (auto& th : pool) th.join();
  util::info("Server stopped.");
  return 0;
}