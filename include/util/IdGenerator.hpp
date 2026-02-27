#pragma once
#include <atomic>
#include <cstdint>

namespace util {
  class IdGenerator {
  public:
    std::uint64_t next() { return ++counter_; }
  private:
    std::atomic<std::uint64_t> counter_{0};
  };
}