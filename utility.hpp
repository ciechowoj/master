#pragma once
#include <functional>
#include <glm>
#include <random>
#include <map>
#include <string>
#include <vector>
#include <iosfwd>

namespace haste {

using std::function;
using std::map;
using std::string;
using std::vector;
using std::pair;
using namespace glm;

string fixedPath(string base, string scene, std::size_t samples);
pair<string, string> splitext(string path);

string make_output_path(
  string input,
  size_t width,
  size_t height,
  size_t num_samples,
  bool snapshot,
  string technique);

std::ostream& pretty_print(std::ostream& stream, const map<string, string>& dict);

std::vector<dvec4> vv3f_to_vv4d(const std::vector<vec3>& data);
std::vector<vec3> vv4f_to_vv3f(std::size_t size, const vec4* data);
std::vector<vec3> vv4d_to_vv3f(std::size_t size, const dvec4* data);

double high_resolution_time();

struct time_scope_t {
  time_scope_t(double& timer) : _timer(timer), _start(high_resolution_time()) {}
  ~time_scope_t() { _timer += high_resolution_time() - _start; }

  time_scope_t(double&&) = delete;
  time_scope_t& operator=(double&&) = delete;

 private:
  double& _timer;
  double _start;
};
}
