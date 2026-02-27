#include "core/RoomManager.hpp"

namespace core {

std::shared_ptr<ChatRoom> RoomManager::get_or_create(const std::string& room_name) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = rooms_.find(room_name);
  if (it != rooms_.end()) return it->second;
  auto room = std::make_shared<ChatRoom>(room_name);
  rooms_[room_name] = room;
  return room;
}

std::shared_ptr<ChatRoom> RoomManager::get(const std::string& room_name) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = rooms_.find(room_name);
  if (it == rooms_.end()) return nullptr;
  return it->second;
}

}