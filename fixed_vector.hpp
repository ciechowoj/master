#pragma once
#include <cstddef>
#include <iterator>
#include <memory>

namespace haste {

template <class T>
void destroy_at(T* object) {
  object->~T();
}

template <class ForwardIterator>
void destroy(ForwardIterator first, ForwardIterator last) {
  for (auto itr = first; itr != last; ++itr) {
    destroy_at(std::addressof(*itr));
  }
}

template <class T, size_t N>
class fixed_vector {
 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  fixed_vector() {}

  fixed_vector(const fixed_vector& that) {
    // using namespace std;

    std::uninitialized_copy(begin(that), end(that), data());
    _size = that._size;
  }

  ~fixed_vector() {
    destroy(begin(), end());
  }

  fixed_vector& operator=(const fixed_vector& that) { return *this; }

  void emplace_back() {
    ++_size;
  }

  void pop_back() {
    --_size;
  }

  iterator begin() { return data(); }

  iterator end() { return data() + size(); }

  reference operator[](size_type index) {
    return data()[index];
  }

  const_reference operator[](size_type index) const {
    return data()[index];
  }

  pointer data() { return reinterpret_cast<pointer>(_storage); }

  const_pointer data() const {
    return reinterpret_cast<const_pointer>(_storage);
  }

  size_type size() const { return _size; }
  size_type capacity() const { return N; }
  size_type fixed_capacity() const { return N; }

 private:
  using _storage_t = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
  _storage_t _storage[N];
  size_type _size = 0;
};

}
