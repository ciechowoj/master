#pragma once
#include <functional>
#include <glm>
#include <random>
#include <string>
#include <vector>

namespace haste {

using std::function;
using std::vector;
using std::string;
using std::pair;
using namespace glm;

struct metadata_t {
  std::string technique;
  size_t num_samples = 0;
  size_t num_basic_rays = 0;
  size_t num_shadow_rays = 0;
  size_t num_tentative_rays = 0;
  size_t num_photons = 0;
  size_t num_scattered = 0;
  size_t num_threads = 0;
  glm::ivec2 resolution = glm::ivec2(0, 0);
  double roulette = 0.0;
  double radius = 0.0;
  double alpha = 0.0;
  double beta = 0.0;
  double epsilon = 0.0;
  double total_time = 0.0;
  double scatter_time = 0.0;
  double build_time = 0.0;
  double gather_time = 0.0;
  double merge_time = 0.0;
  double density_time = 0.0;
  double intersect_time = 0.0;
  double trace_eye_time = 0.0;
  double trace_light_time = 0.0;
  glm::vec3 average = glm::vec3(0.0f, 0.0f, 0.0f);
};

template <class Stream>
Stream& operator<<(Stream& stream, const metadata_t& meta) {
  double connection_time =
      meta.trace_eye_time - meta.trace_light_time - meta.gather_time;
  double query_time = meta.gather_time - meta.merge_time - meta.intersect_time;
  double generate_time = meta.scatter_time - meta.build_time;
  double merge_remaining_time = meta.merge_time - meta.density_time;

  stream << "technique: " << meta.technique << "\n"
         << "num samples: " << meta.num_samples << "\n"
         << "num basic rays: " << meta.num_basic_rays << "\n"
         << "num shadow rays: " << meta.num_shadow_rays << "\n"
         << "num tentative rays: " << meta.num_tentative_rays << "\n"
         << "num photons: " << meta.num_photons << "\n"
         << "num scattered: " << meta.num_scattered / meta.num_samples << " ("
         << (meta.num_scattered / meta.num_samples + meta.num_photons - 1) /
                max(size_t(1), meta.num_photons)
         << "x)\n"
         << "num threads: " << meta.num_threads << "\n"
         << "resolution: [" << meta.resolution.x << ", " << meta.resolution.y
         << "]\n"
         << "roulette: " << meta.roulette << "\n"
         << "radius: " << meta.radius << "\n"
         << "alpha: " << meta.alpha << "\n"
         << "beta: " << meta.beta << "\n"
         << "epsilon: " << meta.epsilon << "\n"
         << "total time: " << meta.total_time << "s\n"
         << "time per sample:        " << meta.total_time / meta.num_samples
         << "s\n"
         << "    trace eye time:       "
         << int(meta.trace_eye_time / meta.total_time * 100) << "%% ("
         << meta.trace_eye_time / meta.num_samples << "s)\n"
         << "        gather time:        "
         << int(meta.gather_time / meta.total_time * 100) << "%% ("
         << meta.gather_time / meta.num_samples << "s)\n"
         << "            query time:       "
         << int(query_time / meta.total_time * 100) << "%% ("
         << query_time / meta.num_samples << "s)\n"
         << "            merge time:       "
         << int(meta.merge_time / meta.total_time * 100) << "%% ("
         << meta.merge_time / meta.num_samples << "s)\n"
         << "                density time:   "
         << int(meta.density_time / meta.total_time * 100) << "%% ("
         << meta.density_time / meta.num_samples << "s)\n"
         << "                rest time:      "
         << int(merge_remaining_time / meta.total_time * 100) << "%% ("
         << merge_remaining_time / meta.num_samples << "s)\n"
         << "        connection time:    "
         << int(connection_time / meta.total_time * 100) << "%% ("
         << connection_time / meta.num_samples << "s)\n"
         << "    scatter time:         "
         << int(meta.scatter_time / meta.total_time * 100) << "%% ("
         << meta.scatter_time / meta.num_samples << "s)\n"
         << "        generate time:      "
         << int(generate_time / meta.total_time * 100) << "%% ("
         << generate_time / meta.num_samples << "s)\n"
         << "        build time:         "
         << int(meta.build_time / meta.total_time * 100) << "%% ("
         << meta.build_time / meta.num_samples << "s)";

  return stream;
}

void saveEXR(const std::string& path, const metadata_t& metadata,
             const vec3* data);

void saveEXR(const std::string& path, const metadata_t& metadata,
             const std::vector<vec3>& data);

void loadEXR(const std::string& path, metadata_t& metadata,
             std::vector<vec3>& data);

std::vector<dvec4> vv3f_to_vv4d(const std::vector<vec3>& data);
std::vector<vec3> vv4f_to_vv3f(std::size_t size, const vec4* data);
std::vector<vec3> vv4d_to_vv3f(std::size_t size, const dvec4* data);

dvec3 computeAVG(const string& path);
void printAVG(const string& path);
void subtract(const string& result, const string& path0, const string& path1);
void merge(const string& result, const string& path0, const string& path1);

struct compute_errors_t {
  double abs;
  double rms;
  double time0;
  double time1;
  string id0;
  string id1;
};

compute_errors_t compute_errors(const string& path0, const string& path1);
void print_errors(const string& path0, const string& path1);

void filter_out_nan(const string& source, const string& target);
void print_time(const string& source);

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
