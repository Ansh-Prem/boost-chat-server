#pragma once
#include <string>
#include <cstdint>

namespace core {
  struct Message {
    std::uint64_t from_id{};
    std::string from_nick;
    std::string room;
    std::string text;        // already formatted payload
    bool is_private{false};
    std::string to_nick;     // for private messages
  };
}