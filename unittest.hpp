#pragma once
#include <source_location.hpp>

namespace haste {

using location_t = source_location_t;

namespace detail {

template <class X, class Y>
constexpr int _same = 0;
template <class X>
constexpr int _same<X, X> = 1;

template <class T, class X, class... Xs>
constexpr int same = _same<T, X>* same<T, Xs...>;

template <class T, class X>
constexpr int same<T, X> = _same<T, X>;

template <int N, class T, class X, class... Xs>
struct _index {
  static const int v = same<T, X> ? N : _index<N + 1, T, Xs...>::v;
};

template <int N, class T, class X>
struct _index<N, T, X> {
  static const int v = same<T, X> ? N : -1;
};

template <class T, class... Xs>
constexpr int index = _index<0, T, Xs...>::v;

template <class T>
constexpr int erase_ivec_type =
    index<T, char, unsigned char, signed char, short, unsigned short, int,
          unsigned int, long, unsigned long, long long, unsigned long long>;

template <class T, class = int>
constexpr int has_x = 0;

template <class T>
constexpr int has_x<T, decltype((void)T::x, 0)> = 1;

template <class T, class = int>
constexpr int has_y = 0;

template <class T>
constexpr int has_y<T, decltype((void)T::y, 0)> = 1;

template <class T, class = int>
constexpr int has_z = 0;

template <class T>
constexpr int has_z<T, decltype((void)T::z, 0)> = 1;

template <class T, class = int>
constexpr int has_w = 0;

template <class T>
constexpr int has_w<T, decltype((void)T::w, 0)> = 1;

template <class T, int X = has_x<T>, int Y = has_y<T>, int Z = has_z<T>,
          int W = has_w<T>>
constexpr int vec_size = X + Y + Z + W;

template <class T>
constexpr int vec_size<T, 0, 0, 0, 0> = 1;

template <class T, int = has_x<T>>
constexpr int fvec_type = index<T, float, double>;

template <class T>
constexpr int fvec_type<T, 1> = index<decltype(T::x), float, double>;

template <class T, int = vec_size<T>, int = has_x<T>>
constexpr bool is_fvec = false;

template <class T, int N>
constexpr bool is_fvec<T, N, 0> = fvec_type<T> != -1;

template <class T>
constexpr bool is_fvec<T, 1, 1> = fvec_type<decltype(T::x)> != -1;

template <class T>
constexpr bool is_fvec<T, 2, 1> = fvec_type<decltype(T::x)> != -1 &&
                                  same<decltype(T::x), decltype(T::y)> &&
                                  sizeof(T) == 2 * sizeof(decltype(T::x));

template <class T>
constexpr bool is_fvec<T, 3, 1> =
    fvec_type<decltype(T::x)> != -1 &&
    same<decltype(T::x), decltype(T::y), decltype(T::z)> &&
    sizeof(T) == 3 * sizeof(decltype(T::x));
;

template <class T>
constexpr bool is_fvec<T, 4, 1> =
    fvec_type<decltype(T::x)> != -1 &&
    same<decltype(T::x), decltype(T::y), decltype(T::z), decltype(T::w)> &&
    sizeof(T) == 4 * sizeof(decltype(T::x));
;

int register_test(void (*)(), int, const char*, const char* = nullptr);

void assert_almost_eq(void*, void*, int, int, const location_t&);

struct num_components_impl {};
}

bool run_all_tests();

#define noinline __attribute__((noinline))

void assert_true(bool x, const location_t& = location_t::current());
void assert_false(bool x, const location_t& = location_t::current());

template <class T, bool = detail::is_fvec<T>>
noinline void assert_almost_eq(
    T a, T b, const location_t& location = location_t::current()) {
  detail::assert_almost_eq(&a, &b, detail::vec_size<T>, detail::fvec_type<T>,
                           location);
}

#undef noinline
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
