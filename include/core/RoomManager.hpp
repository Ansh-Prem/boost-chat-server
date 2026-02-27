#pragma once
#include "core/ChatRoom.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>

namespace core {

  class RoomManager {
  public:
    std::shared_ptr<ChatRoom> get_or_create(const std::string& room_name);
    std::shared_ptr<ChatRoom> get(const std::string& room_name);

  private:
    std::mutex mu_;
    std::unordered_map<std::string, std::shared_ptr<ChatRoom>> rooms_;
  };

}