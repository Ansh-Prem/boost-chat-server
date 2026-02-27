#pragma once
#include <string>

namespace util {

// Keep the class-based API (new)
class Log {
public:
  static void info(const std::string& msg);
  static void warn(const std::string& msg);
  static void error(const std::string& msg);
};

// ✅ Backward-compatible free functions (old code uses these)
inline void info(const std::string& msg)  { Log::info(msg); }
inline void warn(const std::string& msg)  { Log::warn(msg); }
inline void error(const std::string& msg) { Log::error(msg); }

} // namespace util