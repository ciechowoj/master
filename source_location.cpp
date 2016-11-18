#include <execinfo.h>
#include <memory>
#include <source_location.hpp>
#include <string>
#include <algorithm>

namespace haste {

using ulong_t = unsigned long long;

struct source_location_impl_t {
  unsigned line;
  unsigned column;
  std::string file_name;
  std::string function_name;
};

std::string exec(const char* cmd) {
  char buffer[128];
  std::string result = "";
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
    if (fgets(buffer, 128, pipe.get()) != NULL) result += buffer;
  }
  return result;
}

std::string addr2line(const void* addr) {
  char buffer[1024];
  std::sprintf(buffer, "addr2line --exe %s %llx",
               "/home/wojciech/master/build/master/master.bin", (ulong_t)addr);

  return exec(buffer);
}

source_location_t::source_location_t(const source_location_t& that)
  : source_location_t() {
  _addr = that._addr;

  if (that._impl) {
    _impl = new source_location_impl_t(*reinterpret_cast<const source_location_impl_t*>(that._impl));
  }
}

source_location_t::source_location_t(source_location_t&& that)
  : source_location_t() {
    std::swap(_addr, that._addr);
    std::swap(_impl, that._impl);
}

source_location_t::~source_location_t() {
  delete reinterpret_cast<source_location_impl_t*>(_impl);
}

source_location_t source_location_t::current() {
  source_location_t result;
  result._addr = __builtin_return_address(0);
  result._impl = nullptr;
  return result;
}

unsigned source_location_t::line() const {
  _init();
  return reinterpret_cast<const source_location_impl_t*>(_impl)->line;
}

unsigned source_location_t::column() const {
  _init();
  return reinterpret_cast<const source_location_impl_t*>(_impl)->column;
}

const char* source_location_t::file_name() const {
  _init();
  return reinterpret_cast<const source_location_impl_t*>(_impl)->file_name.c_str();
}

const char* source_location_t::function_name() const {
  _init();
  return reinterpret_cast<const source_location_impl_t*>(_impl)->function_name.c_str();
}

void source_location_t::_init() const {
  if (_impl == nullptr) {

    _impl = new source_location_impl_t();
    auto impl = reinterpret_cast<source_location_impl_t*>(_impl);

    auto info = addr2line(_addr);

    auto separator = info.find_last_of(':');

    if (separator == std::string::npos) {
      impl->file_name = info;
    }
    else {
      impl->file_name = info.substr(0, separator);
      impl->line = atoi(info.substr(separator + 1, info.size()).c_str());
    }
  }
}

}
