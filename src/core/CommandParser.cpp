#include "core/CommandParser.hpp"
#include <boost/json.hpp>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace core {
namespace json = boost::json;

static inline std::string trim(std::string s) {
  auto notSpace = [](unsigned char c){ return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
  s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
  return s;
}

static ParsedCommand legacy_parse(const std::string& raw) {
  ParsedCommand cmd;
  std::string line = trim(raw);

  if (line.empty()) { cmd.type = ParsedCommand::Type::Msg; cmd.a.clear(); return cmd; }
  if (line == "/ping") { cmd.type = ParsedCommand::Type::Ping; return cmd; }

  if (line.rfind("/name", 0) == 0) {
    cmd.type = ParsedCommand::Type::SetName;
    std::istringstream iss(line);
    std::string slash; iss >> slash;
    std::getline(iss, cmd.a);
    cmd.a = trim(cmd.a);
    return cmd;
  }

  if (line.rfind("/join", 0) == 0) {
    cmd.type = ParsedCommand::Type::JoinRoom;
    std::istringstream iss(line);
    std::string slash; iss >> slash;
    std::getline(iss, cmd.a);
    cmd.a = trim(cmd.a);
    return cmd;
  }

  if (line.rfind("/pm", 0) == 0) {
    cmd.type = ParsedCommand::Type::PrivateMsg;
    std::istringstream iss(line);
    std::string slash; iss >> slash;
    iss >> cmd.a; // to
    std::getline(iss, cmd.b);
    cmd.b = trim(cmd.b);
    return cmd;
  }

  cmd.type = ParsedCommand::Type::Msg;
  cmd.a = line;
  return cmd;
}

ParsedCommand CommandParser::parse(const std::string& raw) {
  // Try JSON first
  try {
    json::value v = json::parse(raw);
    if (!v.is_object()) return legacy_parse(raw);
    auto const& obj = v.as_object();

    auto itType = obj.find("type");
    if (itType == obj.end() || !itType->value().is_string()) return legacy_parse(raw);

    std::string type = itType->value().as_string().c_str();

    ParsedCommand cmd;

    if (type == "ping") {
      cmd.type = ParsedCommand::Type::Ping;
      return cmd;
    }

    if (type == "name") {
      cmd.type = ParsedCommand::Type::SetName;
      auto it = obj.find("nick");
      cmd.a = (it != obj.end() && it->value().is_string()) ? it->value().as_string().c_str() : "";
      cmd.a = trim(cmd.a);
      return cmd;
    }

    if (type == "join") {
      cmd.type = ParsedCommand::Type::JoinRoom;
      auto it = obj.find("room");
      cmd.a = (it != obj.end() && it->value().is_string()) ? it->value().as_string().c_str() : "";
      cmd.a = trim(cmd.a);
      return cmd;
    }

    if (type == "pm") {
      cmd.type = ParsedCommand::Type::PrivateMsg;
      auto itTo = obj.find("to");
      auto itTx = obj.find("text");
      cmd.a = (itTo != obj.end() && itTo->value().is_string()) ? itTo->value().as_string().c_str() : "";
      cmd.b = (itTx != obj.end() && itTx->value().is_string()) ? itTx->value().as_string().c_str() : "";
      cmd.a = trim(cmd.a);
      cmd.b = trim(cmd.b);
      return cmd;
    }

    if (type == "message") {
      cmd.type = ParsedCommand::Type::Msg;
      auto it = obj.find("text");
      cmd.a = (it != obj.end() && it->value().is_string()) ? it->value().as_string().c_str() : "";
      cmd.a = trim(cmd.a);
      return cmd;
    }

    // Unknown type -> treat as message text fallback
    return legacy_parse(raw);
  } catch (...) {
    // Not JSON -> legacy
    return legacy_parse(raw);
  }
}

} // namespace core