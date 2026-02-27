#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace core {

struct SessionHandle {
  std::uint64_t id{};
  std::string nickname;
  std::function<void(const std::string&)> deliver;
};

class ChatRoom {
public:
  explicit ChatRoom(std::string name);

  const std::string& name() const { return name_; }

  void join(const SessionHandle& s);
  void leave(std::uint64_t id);

  void set_nickname(std::uint64_t id, const std::string& nick);

  void broadcast(const std::string& json_text);
  bool private_message(const std::string& to_nick, const std::string& json_text);

  std::vector<std::string> users() const;

private:
  std::string name_;
  std::unordered_map<std::uint64_t, SessionHandle> sessions_;
};

} // namespace core