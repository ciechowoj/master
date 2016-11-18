#pragma once
#include <source_location.hpp>

namespace haste {
namespace detail {

int register_test(void (*)(), int, const char*, const char* = nullptr);
}

using location_t = source_location_t;

void run_all_tests();

void assert_true(bool x, const location_t& = location_t::current());

template <class A, class B>
__attribute__((noinline)) void assert_almost_eq(const A& a, const B& b,
                      const location_t& location = location_t::current()) {
  assert_true(a == b, location);
}
}

#define unittest_1oxgaer4ai(ln, uq) uq##ln

#define unittest_9w3krm934b(ln, uq, ...)                                      \
  namespace {                                                                 \
  struct unittest_1oxgaer4ai(ln, uq) {                                        \
    static void run();                                                        \
  };                                                                          \
  }                                                                           \
  const int unittest_1oxgaer4ai(ln, uq##_t) = ::haste::detail::register_test( \
      unittest_1oxgaer4ai(ln, uq)::run, ln, __VA_ARGS__);                     \
  void unittest_1oxgaer4ai(ln, uq)::run()

#define unittest(...) \
  unittest_9w3krm934b(__LINE__, unittest_sgv1fsnhkn, __FILE__ __VA_ARGS__)
