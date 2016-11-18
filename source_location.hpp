#pragma once

namespace haste {

struct source_location_t {
  source_location_t() = default;
  source_location_t(const source_location_t&);
  source_location_t(source_location_t&&);
  ~source_location_t();

  static source_location_t current();

  source_location_t& operator=(const source_location_t&) = delete;
  source_location_t& operator=(source_location_t&&) = delete;

  unsigned line() const;
  unsigned column() const;
  const char* file_name() const;
  const char* function_name() const;

 private:
 	void _init() const;
  void* _addr = nullptr;
  mutable void* _impl = nullptr;
};


}
