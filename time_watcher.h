#ifndef HASH_TIME_WATCHER_H_
#define HASH_TIME_WATCHER_H_
#include <chrono>

using namespace std;
using namespace std::chrono;

namespace hard_core {

class TimeWatcher final {
 public:
  // 清除计时器
  TimeWatcher() : start_(system_clock::time_point::min()) {}
  // 清除计时器
  void Clear() { start_ = system_clock::time_point::min(); }
  // 如果计时器正在计时，则返回true
  bool IsStarted() const {
    return (start_.time_since_epoch() != system_clock::duration(0));
  }
  // 启动计时器
  void Start() { start_ = system_clock::now(); }
  // 得到自计时开始后的纳秒值
  uint64_t GetMs() {
    if (IsStarted()) {
      system_clock::duration diff;
      diff = system_clock::now() - start_;
      return duration_cast<nanoseconds>(diff).count();
    }
    return 0;
  }

 private:
  system_clock::time_point start_;
};
}  // namespace hard_core

#endif
