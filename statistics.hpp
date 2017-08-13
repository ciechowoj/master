#pragma once
#include <iosfwd>
#include <map>
#include <string>
#include <vector>
#include <glm>

namespace haste {

using std::map;
using std::string;
using std::vector;
using glm::vec3;

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
	  size_t sample_index = 0;
    float rms_error;
    float abs_error;
    float clock_time;
    float frame_duration;
    size_t numeric_errors;
  };

  struct measurement_t {
    size_t sample_index = 0;
    int pixel_x = -1;
    int pixel_y = -1;
    vec3 value;
  };

  vector<record_t> records;
  vector<measurement_t> measurements;

  statistics_t() = default;
  statistics_t(const map<string, string>& dict);

  map<string, string> to_dict() const;
};

std::ostream& operator<<(std::ostream& stream, const statistics_t& meta);

void print_frame_summary(std::ostream& stream, const statistics_t& statistics);
void print_records_tabular(std::ostream& stream, const statistics_t& statistics);

}
