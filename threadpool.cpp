#include <threadpool.hpp>
#include <stdexcept>

namespace haste {

threadpool_t::threadpool_t() {
  size_t num_cores = std::thread::hardware_concurrency();

  _threads = std::vector<_thread_t>(num_cores);

  size_t index = 0;
  for (auto&& thread : _threads) {
    thread.thread = std::thread(
      [&thread, index, this]() {
        while (!_terminate) {
            std::unique_lock<std::mutex> run_lock(_run_mutex);
            _run.wait(run_lock);

            if (!_terminate) {
              std::unique_lock<std::mutex> done_lock(_done_mutex);

            // do stuff

            }

            thread.done.notify_one();
        }
      });

    ++index;
  }
}

threadpool_t::~threadpool_t() {
  {
    std::unique_lock<std::mutex> run_lock(_run_mutex);
    _terminate = true;
  }

  _run.notify_all();
}

void threadpool_t::run_and_wait(const std::vector<std::function<void()>>& tasks) {

  _tasks = &tasks;

  size_t num_runs = tasks.size() / num_threads();
  size_t remainder = tasks.size() % num_threads();

  _offset = 0;

  while (_offset <= num_runs) {
    _run.notify_all();

    for (auto&& thread : _threads) {
      std::unique_lock<std::mutex> lock(_done_mutex);
      thread.done.wait(lock);
    }

    std::unique_lock<std::mutex> lock(_run_mutex);
    ++_offset;
  }
}

size_t threadpool_t::num_threads() {
  return _threads.size();
}


}
