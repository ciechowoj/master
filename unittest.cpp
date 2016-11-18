#include <cstdio>
#include <new>
#include <unittest.hpp>

namespace haste {
namespace detail {

struct trigger_t {
  trigger_t();
  trigger_t(trigger_t&&) = delete;
  ~trigger_t();

  static trigger_t& enlist(void (*)(), int, const char*);

  char _impl[32];
};

int register_test(void (*test)(), int line, const char* file, const char*) {
  trigger_t::enlist(test, line, file);
  return -1;
}

namespace {
trigger_t& instance() {
  static trigger_t instance;
  return instance;
}
}

using ulong_t = unsigned long long;

struct trigger_impl_t {
  using test_t = void (*)();
  test_t* tests = nullptr;
  ulong_t capacity = 0;
  ulong_t size = 0;
};

trigger_t::trigger_t() {
  static_assert(sizeof(trigger_impl_t) <= sizeof(_impl), "assertion failed");
  new (_impl) trigger_impl_t();
}

trigger_t::~trigger_t() {
  auto impl = reinterpret_cast<trigger_impl_t*>(_impl);
  delete[] impl->tests;
  impl->~trigger_impl_t();
}

trigger_t& trigger_t::enlist(void (*test)(), int, const char*) {
  auto& instance = detail::instance();
  auto impl = reinterpret_cast<trigger_impl_t*>(instance._impl);

  if (impl->size == impl->capacity) {
    auto capacity = impl->capacity * 2;
    capacity = capacity < 16 ? 16 : capacity;

    auto tests = new trigger_impl_t::test_t[capacity];

    for (ulong_t i = 0; i < impl->size; ++i) {
      tests[i] = impl->tests[i];
    }

    delete[] impl->tests;

    impl->tests = tests;
    impl->capacity = capacity;
  }

  impl->tests[impl->size] = test;
  ++impl->size;

  return instance;
}
}

void run_all_tests() {
  using namespace ::haste::detail;

  auto impl = reinterpret_cast<trigger_impl_t*>(detail::instance()._impl);

  for (ulong_t i = 0; i < impl->size; ++i) {
    impl->tests[i]();
  }
}

void assert_true(bool x, const location_t& location) {
  if (!x) {
    std::printf("%s:%u: assertion failed\n", location.file_name(),
                location.line());
    std::fflush(stdout);
  }
}
}
