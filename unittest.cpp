#include <cstdio>
#include <glm>
#include <new>
#include <unittest.hpp>

namespace haste {
namespace detail {

unittest() {
  static_assert(is_fvec<float>, "");
  static_assert(!is_fvec<void*>, "");
  static_assert(!is_fvec<ivec2>, "");
  static_assert(!is_fvec<ivec3>, "");
  static_assert(!is_fvec<ivec4>, "");
  static_assert(is_fvec<vec4>, "");

  static_assert(has_x<vec2>::v == 1, "");
  static_assert(has_x<vec3>::v == 1, "");
  static_assert(has_x<ivec2>::v == 1, "");
  static_assert(!has_x<int>::v, "");

  static_assert(vec_size<float> == 1, "");
  static_assert(vec_size<int> == 1, "");
  static_assert(vec_size<vec2> == 2, "");
  static_assert(vec_size<vec3> == 3, "");
  static_assert(vec_size<vec4> == 4, "");
  static_assert(vec_size<dvec4> == 4, "");
}

unittest() {
  static_assert(index<short, int> == -1, "");
  static_assert(index<int, int> == 0, "");
  static_assert(index<int, float, int> == 1, "");
  static_assert(index<int, float, double, int> == 2, "");
  static_assert(index<int, float, double, int, bool> == 2, "");
}

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

unsigned ulp_dist(float a, float b) {
  union conv_t {
    float f;
    int i;
    static_assert(sizeof(int) == sizeof(float), "");
  };

  conv_t ca, cb;
  ca.f = a;
  cb.f = b;

  if (ca.i < 0 != cb.i < 0) {
    return unsigned(abs(ca.i & ~(1 << 31))) + unsigned(abs(cb.i & ~(1 << 31)));
  } else {
    return abs(ca.i - cb.i);
  }
}

bool almost_eq(float a, float b) {
  return abs(a - b) < FLT_EPSILON || ulp_dist(a, b) < 64;
}

unittest() {
  assert_true(ulp_dist(0.0f, -0.0f) == 0);
  assert_true(ulp_dist(1.0000001f, 1.0000002f) != 0);
}

void print_vector(float* v, int s) {
  if (s == 1) {
    std::printf("%f", v[0]);
  } else {
    std::printf("[%f", v[0]);

    for (size_t i = 1; i < s; ++i) {
      std::printf(", %f", v[i]);
    }

    std::printf("]");
  }
}

namespace detail {

void assert_almost_eq(void* a, void* b, int size, int type,
                      const location_t& location) {
  if (type == 0) {
    auto fa = reinterpret_cast<float*>(a);
    auto fb = reinterpret_cast<float*>(b);

    for (int i = 0; i < size; ++i) {
      if (!almost_eq(fa[i], fb[i])) {
        std::printf("%s:%u: assert_almost_eq(",
                    location.file_name(), location.line());

        print_vector(fa, size);
        std::printf(", ");
        print_vector(fb, size);
        std::printf(") failed\n");

        return;
      }
    }
  } else if (type == 1) {
  } else {
    // not gonna happen
  }
}
}

}
