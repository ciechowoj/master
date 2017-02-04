#include <cstdio>
#include <glm>
#include <new>
#include <unittest>

namespace haste {
namespace detail {

unittest() {
  static_assert(is_fvec<float>, "");
  static_assert(!is_fvec<void*>, "");
  static_assert(!is_fvec<ivec2>, "");
  static_assert(!is_fvec<ivec3>, "");
  static_assert(!is_fvec<ivec4>, "");
  static_assert(is_fvec<vec4>, "");

  static_assert(has_x<vec2> == 1, "");
  static_assert(has_x<vec3> == 1, "");
  static_assert(has_x<ivec2> == 1, "");
  static_assert(!has_x<int>, "");

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
}

using ulong_t = unsigned long long;

struct context_t {
  ulong_t num_failed = 0;
  bool passed = true;

  void reset() { passed = true; }

  void assert_failed() { passed = false; }
};

struct trigger_t {
  trigger_t() = default;
  trigger_t(trigger_t&&) = delete;
  ~trigger_t();

  trigger_t& enlist(void (*)(), int, const char*);

  using test_t = void (*)();
  test_t* _tests = nullptr;
  ulong_t _capacity = 0;
  ulong_t _size = 0;

  static thread_local context_t* context;
};

thread_local context_t* trigger_t::context = nullptr;

namespace {
trigger_t& instance() {
  static trigger_t instance;
  return instance;
}

context_t* context_instance() { return instance().context; }
}

namespace detail {

int register_test(void (*test)(), int line, const char* file, const char*) {
  instance().enlist(test, line, file);
  return -1;
}
}

trigger_t::~trigger_t() { delete[] _tests; }

trigger_t& trigger_t::enlist(void (*test)(), int, const char*) {
  if (_size == _capacity) {
    auto capacity = _capacity * 2;
    capacity = capacity < 16 ? 16 : capacity;

    auto tests = new test_t[capacity];

    for (ulong_t i = 0; i < _size; ++i) {
      tests[i] = _tests[i];
    }

    delete[] _tests;

    _tests = tests;
    _capacity = capacity;
  }

  _tests[_size] = test;
  ++_size;

  return *this;
}

bool run_all_tests() {
  auto& trigger = instance();

  std::printf("Running %llu unittest...\n", trigger._size);

  context_t context;
  trigger.context = &context;

  for (ulong_t i = 0; i < trigger._size; ++i) {
    context.reset();
    trigger._tests[i]();
    if (!context.passed) {
      ++context.num_failed;
    }
  }

  if (context.num_failed) {
    printf("%llu tests failed.\n", context.num_failed);
  }
  else {
    printf("All tests succeeded.\n");
  }

  return context.num_failed == 0;
}

void assert_true(bool x, const location_t& location) {
  if (!x) {
    std::printf("%s:%u: assertion failed\n", location.file_name(),
                location.line());
    std::fflush(stdout);

    context_instance()->assert_failed();
  }
}

void assert_false(bool x, const location_t& location) {
  if (x) {
    std::printf("%s:%u: assertion failed\n", location.file_name(),
                location.line());
    std::fflush(stdout);

    context_instance()->assert_failed();
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

  if ((ca.i < 0) != (cb.i < 0)) {
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
  assert_false(almost_eq(1.0f, -1.0f));
  assert_true(almost_eq(1.0f, 1.0f));
}

void print_vector(float* v, int s) {
  if (s == 1) {
    std::printf("%f", v[0]);
  } else {
    std::printf("[%f", v[0]);

    for (int i = 1; i < s; ++i) {
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
        std::printf("%s:%u: assert_almost_eq(", location.file_name(),
                    location.line());

        print_vector(fa, size);
        std::printf(", ");
        print_vector(fb, size);
        std::printf(") failed\n");

        context_instance()->assert_failed();

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
