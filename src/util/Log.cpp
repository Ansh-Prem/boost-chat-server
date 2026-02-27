#include "util/Log.hpp"
#include <boost/json.hpp>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace util {
namespace json = boost::json;

static std::string now_iso8601() {
  using namespace std::chrono;
  auto now = system_clock::now();
  std::time_t t = system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
  return oss.str();
}

static void log_json(const char* level, const std::string& msg) {
  json::object o;
  o["ts"] = now_iso8601();
  o["level"] = level;
  o["msg"] = msg;
  std::cout << json::serialize(o) << std::endl;
}

void Log::info(const std::string& msg)  { log_json("INFO", msg); }
void Log::warn(const std::string& msg)  { log_json("WARN", msg); }
void Log::error(const std::string& msg) { log_json("ERROR", msg); }

} // namespace util