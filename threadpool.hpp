#pragma once
#include <cstddef>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>

namespace haste {

using std::size_t;

class threadpool_t {
public:
  threadpool_t();
  threadpool_t(const threadpool_t&) = delete;
  ~threadpool_t();

  threadpool_t& operator=(const threadpool_t&) = delete;

  void run_and_wait(const std::vector<std::function<void()>>& tasks);
  size_t num_threads();
private:
  struct _thread_t {
    std::thread thread;
    std::condition_variable done;
  };

  std::vector<_thread_t> _threads;
  const std::vector<std::function<void()>>* _tasks;
  std::mutex _run_mutex;
  std::mutex _done_mutex;
  std::condition_variable _run;
  std::atomic<bool> _terminate;
  std::atomic<size_t> _offset;
};

}

