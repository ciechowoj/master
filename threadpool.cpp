#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <threadpool.hpp>

namespace haste {

struct data_queue_thunk_t {
  unsigned size;
  char data[sizeof(size)];
};

struct data_queue_impl_t {
  std::mutex mutex;
  std::condition_variable full_condition;
  std::condition_variable empty_condition;
  char* buffer = nullptr;
  size_t capacity = 0;
  size_t head = 0;
  size_t tail = 0;
  size_t last = 0;

  ~data_queue_impl_t() {
    if (buffer) {
      std::free(buffer);
    }
  }

  size_t thunk_size(size_t thunk_size) {
    return (sizeof(data_queue_thunk_t) - sizeof(data_queue_thunk_t::data) +
            thunk_size + 7) /
           8 * 8;
  }

  bool is_empty() const { return head == tail; }

  bool is_full(size_t thunk_size) const {
    if (tail < head) {
      return head - tail <= thunk_size;
    } else {
      return capacity < tail + thunk_size ? head <= thunk_size : false;
    }
  }

  data_queue_thunk_t* acquire_thunk(size_t thunk_size) {
    if (buffer == nullptr) {
      buffer = reinterpret_cast<char*>(std::malloc(capacity));
    }

    if (capacity < tail + thunk_size) {
      last = tail;
      tail = 0;
    }

    auto result = reinterpret_cast<data_queue_thunk_t*>(buffer + tail);
    tail += thunk_size;
    return result;
  }

  data_queue_thunk_t* release_thunk() {
    if (head == last) {
      head = 0;
      last = capacity;
    }

    auto thunk = reinterpret_cast<data_queue_thunk_t*>(buffer + head);
    head += thunk_size(thunk->size);
    return thunk;
  }
};

data_queue_t::data_queue_t(size_t capacity) {
  static_assert(sizeof(data_queue_impl_t) <= sizeof(_impl), "assertion failed");
  auto impl = reinterpret_cast<data_queue_impl_t*>(_impl);
  new (impl) data_queue_impl_t();

  impl->capacity = capacity;
  impl->last = capacity;
}

data_queue_t::~data_queue_t() {
  auto impl = reinterpret_cast<data_queue_impl_t*>(_impl);
  impl->~data_queue_impl_t();
}

bool data_queue_t::empty() const {
  auto impl = reinterpret_cast<const data_queue_impl_t*>(_impl);
  return impl->is_empty();
}

void data_queue_t::_push(size_t size, void* closure,
                         void (*callback)(void*, char*)) {
  auto impl = reinterpret_cast<data_queue_impl_t*>(_impl);
  const auto thunk_size = impl->thunk_size(size);

  std::unique_lock<std::mutex> lock(impl->mutex);
  impl->full_condition.wait(lock, [=] { return !impl->is_full(thunk_size); });

  auto thunk = impl->acquire_thunk(thunk_size);
  thunk->size = size;
  callback(closure, thunk->data);

  impl->empty_condition.notify_one();
}

void data_queue_t::_pop(void* closure, void (*callback)(void*, size_t, char*)) {
  auto impl = reinterpret_cast<data_queue_impl_t*>(_impl);

  std::unique_lock<std::mutex> lock(impl->mutex);
  impl->empty_condition.wait(lock, [=] { return !impl->is_empty(); });

  auto thunk = impl->release_thunk();
  callback(closure, thunk->size, thunk->data);

  impl->full_condition.notify_one();
}

task_queue_t::task_queue_t(size_t capacity) : _queue(capacity) {}

task_queue_t::~task_queue_t() {
  while (!empty()) {
    skip();
  }
}

void task_queue_t::exec() {
  char closure[1024];
  void (*exec)(char*) = nullptr;
  void (*destruct)(char*) = nullptr;

  _queue.pop([&](size_t size, char* data) {
    auto thunk = reinterpret_cast<_thunk_t*>(data);
    exec = thunk->exec;
    destruct = thunk->destruct;
    thunk->move(closure, thunk->data);
    thunk->destruct(thunk->data);
  });

  exec(closure);
  destruct(closure);
}

void task_queue_t::skip() {
  _queue.pop([&](size_t size, char* data) {
    auto thunk = reinterpret_cast<_thunk_t*>(data);
    thunk->destruct(thunk->data);
  });
}

bool task_queue_t::empty() const { return _queue.empty(); }

size_t default_num_cores() {
  size_t num_cores = std::thread::hardware_concurrency();
  return num_cores == 0 ? 4 : num_cores;
}

threadpool_t::threadpool_t(size_t num_threads) {
  num_threads = num_threads == 0 ? default_num_cores() : num_threads;

  _threads = std::vector<std::thread>(num_threads);

  _terminate = false;

  for (auto&& thread : _threads) {
    thread = std::thread([this]() {
      while (!_terminate) {
        _queue.exec();
      }
    });
  }
}

threadpool_t::~threadpool_t() {
  _terminate = true;

  for (size_t i = 0; i < num_threads(); ++i) {
    _queue.push([] {});
  }

  for (size_t i = 0; i < num_threads(); ++i) {
    _threads[i].join();
  }
}

size_t threadpool_t::num_threads() { return _threads.size(); }

namespace detail {

void exec2d(threadpool_t& pool, size_t width, size_t height, size_t batch,
            void* closure,
            void (*callback)(void*, size_t, size_t, size_t, size_t)) {
  size_t num_cols = (width + batch - 1) / batch;
  size_t num_rows = (height + batch - 1) / batch;
  size_t num_cells = num_cols * num_rows;

  std::mutex mutex;
  std::condition_variable condition;
  std::atomic<size_t> counter(0);

  for (size_t col = 0; col < num_cols; ++col) {
    for (size_t row = 0; row < num_rows; ++row) {
      pool.exec([=, &mutex, &counter, &condition] {
        size_t x0 = col * batch;
        size_t x1 = std::min(width, x0 + batch);
        size_t y0 = row * batch;
        size_t y1 = std::min(height, y0 + batch);
        callback(closure, x0, x1, y0, y1);

        if (counter.fetch_add(1) == num_cells - 1) {
          std::unique_lock<std::mutex> lock(mutex);
          condition.notify_one();
        }
      });
    }
  }

  std::unique_lock<std::mutex> lock(mutex);
  condition.wait(lock, [&] { return counter == num_cells; });
}
}
}
