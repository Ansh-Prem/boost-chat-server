#include "core/ChatRoom.hpp"
#include <algorithm>

namespace core {

ChatRoom::ChatRoom(std::string name) : name_(std::move(name)) {}

void ChatRoom::join(const SessionHandle& s) {
  sessions_[s.id] = s;
}

void ChatRoom::leave(std::uint64_t id) {
  sessions_.erase(id);
}

void ChatRoom::set_nickname(std::uint64_t id, const std::string& nick) {
  auto it = sessions_.find(id);
  if (it != sessions_.end()) it->second.nickname = nick;
}

void ChatRoom::broadcast(const std::string& json_text) {
  for (auto const& [id, s] : sessions_) {
    if (s.deliver) s.deliver(json_text);
  }
}

bool ChatRoom::private_message(const std::string& to_nick, const std::string& json_text) {
  for (auto& [id, s] : sessions_) {
    if (s.nickname == to_nick) {
      if (s.deliver) s.deliver(json_text);
      return true;
    }
  }
  return false;
}

std::vector<std::string> ChatRoom::users() const {
  std::vector<std::string> out;
  out.reserve(sessions_.size());
  for (auto const& [id, s] : sessions_) out.push_back(s.nickname);
  std::sort(out.begin(), out.end());
  return out;
}

} // namespace core