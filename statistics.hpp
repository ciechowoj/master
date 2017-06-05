#pragma once
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace haste {

using std::map;
using std::string;
using std::vector;

struct statistics_t {
  size_t num_samples = 0;
  size_t num_basic_rays = 0;
  size_t num_shadow_rays = 0;
  size_t num_tentative_rays = 0;
  size_t num_photons = 0;
  size_t num_scattered = 0;
  double total_time = 0.0;
  double scatter_time = 0.0;
  double build_time = 0.0;
  double gather_time = 0.0;
  double merge_time = 0.0;
  double density_time = 0.0;
  double intersect_time = 0.0;
  double trace_eye_time = 0.0;
  double trace_light_time = 0.0;

  struct record_t {
    float rms_error;
    float abs_error;
    float clock_time;
    float frame_duration;
    size_t numeric_errors;
  };

  vector<record_t> records;

  statistics_t() = default;
  statistics_t(const map<string, string>& dict);

  map<string, string> to_dict() const;
};

std::ostream& operator<<(std::ostream& stream, const statistics_t& meta);

}
