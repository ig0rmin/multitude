#pragma once

#include <folly/futures/Timekeeper.h>
#include <folly/futures/Future.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstdint>
#include <map>

namespace folly { namespace detail {

/// Very simple timekeeper that keeps pending events in an ordered map
class SimpleTimekeeper : public Timekeeper {
 public:
  SimpleTimekeeper();
  ~SimpleTimekeeper();

  Future<Unit> after(Duration) override;

 private:
  struct Key {
    std::chrono::steady_clock::time_point time;
    uint64_t counter;

    bool operator<(const Key & rhs) const noexcept { 
      return time < rhs.time || (time == rhs.time && counter < rhs.counter); 
    }
  };

  void threadLoop();
  void setValues(std::chrono::time_point<std::chrono::steady_clock> now);

  std::atomic<uint64_t> counter_;
  std::atomic<bool> continue_;
  std::mutex mutex_;
  std::map<Key, Promise<Unit>> pending_;
  std::thread thread_;
};

Timekeeper* getTimekeeperSingleton();

}  // namespace detail
}  // namespace folly
