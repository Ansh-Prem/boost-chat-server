#pragma once
#include <string>

namespace core {

struct ParsedCommand {
  enum class Type {
    Msg,       // {"type":"message","text":"..."}
    SetName,   // {"type":"name","nick":"..."}
    JoinRoom,  // {"type":"join","room":"..."}
    PrivateMsg,// {"type":"pm","to":"...","text":"..."}
    Ping       // {"type":"ping"}
  } type{Type::Msg};

  std::string a; // nick/room/to/text
  std::string b; // pm text
};

// Parses JSON messages from client.
// Also supports legacy text commands (/join, /name, /pm, /ping, plain text) as fallback.
class CommandParser {
public:
  static ParsedCommand parse(const std::string& line);
};

} // namespace core