#include "net/WsSession.hpp"
#include <utility>
#include <string>
#include <ctime>

#include <boost/json.hpp>
#include "util/Log.hpp"

namespace net {
namespace json = boost::json;

static std::string jdump(const json::object& o) {
  return json::serialize(o);
}

static json::object mk_system(const std::string& room, const std::string& text) {
  json::object o;
  o["type"] = "system";
  o["room"] = room;
  o["text"] = text;
  o["ts"] = std::int64_t(std::time(nullptr));
  return o;
}

static json::object mk_error(const std::string& room, const std::string& text) {
  json::object o;
  o["type"] = "error";
  o["room"] = room;
  o["text"] = text;
  o["ts"] = std::int64_t(std::time(nullptr));
  return o;
}

static json::object mk_users(const std::string& room, const std::vector<std::string>& users) {
  json::object o;
  o["type"] = "users";
  o["room"] = room;
  json::array arr;
  for (auto const& u : users) arr.push_back(json::value(u)); // ✅ FIX
  o["users"] = std::move(arr);
  o["ts"] = std::int64_t(std::time(nullptr));
  return o;
}

static json::object mk_msg(const std::string& room, const std::string& from, const std::string& text) {
  json::object o;
  o["type"] = "message";
  o["room"] = room;
  o["from"] = from;
  o["text"] = text;
  o["ts"] = std::int64_t(std::time(nullptr));
  return o;
}

static json::object mk_pm(const std::string& room, const std::string& from, const std::string& to, const std::string& text) {
  json::object o;
  o["type"] = "pm";
  o["room"] = room;
  o["from"] = from;
  o["to"] = to;
  o["text"] = text;
  o["ts"] = std::int64_t(std::time(nullptr));
  return o;
}

WsSession::WsSession(tcp::socket socket, core::RoomManager& rooms, std::uint64_t id)
  : ws_(std::move(socket)),
    strand_(asio::make_strand(static_cast<asio::io_context&>(ws_.get_executor().context()))),
    rooms_(rooms),
    id_(id) {
  nick_ = "user#" + std::to_string(id_);
}

std::string WsSession::display_name() const {
  return nick_.empty() ? ("user#" + std::to_string(id_)) : nick_;
}

void WsSession::start() {
  auto self = shared_from_this();

  ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
  ws_.async_accept(asio::bind_executor(strand_, [this, self](beast::error_code ec) {
    if (ec) {
      util::Log::error("ws_accept_failed id=" + std::to_string(id_));
      return;
    }

    util::Log::info("ws_connected id=" + std::to_string(id_));
    join_room("lobby");
    deliver(jdump(mk_system("lobby", "Welcome! JSON protocol: {type: join/name/message/pm/ping}")));
    do_read();
  }));
}

void WsSession::join_room(const std::string& room_name) {
  auto self = shared_from_this();
  asio::dispatch(strand_, [this, self, room_name] {
    util::Log::info("ws_join id=" + std::to_string(id_) + " room=" + room_name);

    if (room_) room_->leave(id_);
    room_ = rooms_.get_or_create(room_name);

    core::SessionHandle h;
    h.id = id_;
    h.nickname = nick_;
    h.deliver = [wself = std::weak_ptr<WsSession>(self)](const std::string& m) {
      if (auto s = wself.lock()) s->deliver(m);
    };

    room_->join(h);

    room_->broadcast(jdump(mk_system(room_->name(), display_name() + " joined")));
    room_->broadcast(jdump(mk_users(room_->name(), room_->users())));
  });
}

void WsSession::do_read() {
  auto self = shared_from_this();
  ws_.async_read(buffer_, asio::bind_executor(strand_, [this, self](beast::error_code ec, std::size_t) {
    if (ec) {
      util::Log::info("ws_disconnected id=" + std::to_string(id_));
      if (room_) {
        room_->broadcast(jdump(mk_system(room_->name(), display_name() + " disconnected")));
        room_->leave(id_);
        room_->broadcast(jdump(mk_users(room_->name(), room_->users())));
      }
      return;
    }

    std::string raw = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());

    util::Log::info("ws_rx id=" + std::to_string(id_) + " bytes=" + std::to_string(raw.size()));

    if (!room_) join_room("lobby");

    auto cmd = core::CommandParser::parse(raw);

    switch (cmd.type) {
      case core::ParsedCommand::Type::Ping: {
        json::object o;
        o["type"] = "pong";
        o["ts"] = std::int64_t(std::time(nullptr));
        deliver(jdump(o));
        util::Log::info("ws_ping_pong id=" + std::to_string(id_));
        break;
      }

      case core::ParsedCommand::Type::SetName: {
        if (cmd.a.empty()) { deliver(jdump(mk_error(room_->name(), "missing nick"))); break; }
        std::string old = display_name();
        nick_ = cmd.a;
        room_->set_nickname(id_, nick_);

        util::Log::info("ws_name id=" + std::to_string(id_) + " nick=" + nick_);
        room_->broadcast(jdump(mk_system(room_->name(), old + " is now " + display_name())));
        room_->broadcast(jdump(mk_users(room_->name(), room_->users())));
        break;
      }

      case core::ParsedCommand::Type::JoinRoom: {
        if (cmd.a.empty()) { deliver(jdump(mk_error(room_->name(), "missing room"))); break; }
        join_room(cmd.a);
        break;
      }

      case core::ParsedCommand::Type::PrivateMsg: {
        if (cmd.a.empty() || cmd.b.empty()) { deliver(jdump(mk_error(room_->name(), "pm requires {to,text}"))); break; }
        auto payload = jdump(mk_pm(room_->name(), display_name(), cmd.a, cmd.b));
        bool ok = room_->private_message(cmd.a, payload);

        util::Log::info("ws_pm id=" + std::to_string(id_) + " to=" + cmd.a + " ok=" + (ok ? "true" : "false"));

        if (!ok) deliver(jdump(mk_error(room_->name(), "user_not_found: " + cmd.a)));
        else deliver(payload); // echo to sender
        break;
      }

      case core::ParsedCommand::Type::Msg:
      default: {
        if (cmd.a.empty()) break;
        util::Log::info("ws_msg id=" + std::to_string(id_) + " room=" + room_->name());
        room_->broadcast(jdump(mk_msg(room_->name(), display_name(), cmd.a)));
        break;
      }
    }

    do_read();
  }));
}

void WsSession::deliver(const std::string& msg) {
  auto self = shared_from_this();
  asio::post(strand_, [this, self, msg] {
    if (outq_.size() >= MAX_QUEUE) outq_.pop_front();
    bool writing = !outq_.empty();
    outq_.push_back(msg);
    if (!writing) do_write();
  });
}

void WsSession::do_write() {
  auto self = shared_from_this();
  ws_.text(true);

  auto const& s = std::as_const(outq_.front());
  ws_.async_write(asio::buffer(s),
    asio::bind_executor(strand_, [this, self](beast::error_code ec, std::size_t) {
      if (ec) {
        util::Log::warn("ws_write_error id=" + std::to_string(id_));
        if (room_) room_->leave(id_);
        return;
      }
      outq_.pop_front();
      if (!outq_.empty()) do_write();
    })
  );
}

} // namespace net