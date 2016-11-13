#pragma once
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <thread>
#include <vector>

namespace haste {

using std::size_t;

class data_queue_t {
 public:
  data_queue_t(size_t capacity = 1024);
  data_queue_t(const data_queue_t&) = delete;
  ~data_queue_t();

  data_queue_t operator=(const data_queue_t&) = delete;

  template <class F>
  void push(size_t size, F&& callback) {
    _push(size, &callback,
          [](void* closure, char* data) { (*(F*)(closure))(data); });
  }

  template <class F>
  void pop(F&& callback) {
    _pop(&callback, [](void* closure, size_t size, char* data) {
      (*(F*)(closure))(size, data);
    });
  }

  bool empty() const;

 private:
  void _push(size_t, void*, void (*)(void*, char*));
  void _pop(void*, void (*)(void*, size_t, char*));
  char _impl[256];
};

class task_queue_t {
 public:
  task_queue_t(size_t capacity = 1024);
  ~task_queue_t();

  template <class F>
  void push(F&& task) {
    using Closure = typename std::decay<F>::type;

    constexpr size_t thunk_size =
        sizeof(_thunk_t) - sizeof(_thunk_t::data) + sizeof(F);

    _queue.push(thunk_size, [=](char* data) {
      auto thunk = reinterpret_cast<_thunk_t*>(data);

      thunk->exec = [](char* closure) { (*(Closure*)(closure))(); };

      thunk->move = [](char* dst, char* src) {
        new (dst) Closure(std::move(*(Closure*)(src)));
      };

      thunk->destruct = [](char* closure) {
        ((Closure*)(closure))->~Closure();
      };

      thunk->move(thunk->data, (char*)&task);
    });
  }

  void exec();
  void skip();
  bool empty() const;

 private:
  data_queue_t _queue;

  struct _thunk_t {
    void (*exec)(char* data);
    void (*move)(char* from, char* to);
    void (*destruct)(char* data);
    char data[8];
  };
};

class threadpool_t {
 public:
  threadpool_t();
  threadpool_t(const threadpool_t&) = delete;
  ~threadpool_t();

  threadpool_t& operator=(const threadpool_t&) = delete;

  template <class F>
  void exec(F&& task) {
    _queue.push(task);
  }

  size_t num_threads();

 private:
  std::atomic<bool> _terminate;
  std::vector<std::thread> _threads;
  task_queue_t _queue;
};

namespace detail {

void exec2d(threadpool_t&, size_t, size_t, size_t, void*,
            void (*)(void*, size_t, size_t, size_t, size_t));
}

template <class F>
void exec2d(threadpool_t& pool, size_t width, size_t height, size_t batch,
            F&& task) {
  detail::exec2d(pool, width, height, batch, &task,
                 [](void* closure, size_t x0, size_t x1, size_t y0, size_t y1) {
                   using Closure = typename std::decay<F>::type;
                   (*reinterpret_cast<Closure*>(closure))(x0, x1, y0, y1);
                 });
}
}
