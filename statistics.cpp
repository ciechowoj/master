#include <algorithm>
#include <iostream>
#include <statistics.hpp>

namespace haste {

bool startswith(const std::string& s, const std::string& p) {
  const char* s_itr = s.c_str();
  const char* p_itr = p.c_str();

  while (*s_itr != '\0' && *p_itr != '\0' && *s_itr == *p_itr) {
    ++s_itr;
    ++p_itr;
  }

  return *p_itr == '\0';
}

bool endswith(const std::string& s, const std::string& p) {
  const char* s_rbegin = s.c_str() - 1;
  const char* p_rbegin = p.c_str() - 1;
  const char* s_itr = s.c_str() + s.size() - 1;
  const char* p_itr = p.c_str() + p.size() - 1;

  while (s_itr != s_rbegin && p_itr != p_rbegin && *s_itr == *p_itr) {
    --s_itr;
    --p_itr;
  }

  return p_itr == p_rbegin;
}


statistics_t::statistics_t(const map<string, string>& dict) {
	num_samples = stoll(dict.find("statistics.num_samples")->second);
	num_basic_rays = stoll(dict.find("statistics.num_basic_rays")->second);
	num_shadow_rays = stoll(dict.find("statistics.num_shadow_rays")->second);
	num_tentative_rays = stoll(dict.find("statistics.num_tentative_rays")->second);
	num_photons = stoll(dict.find("statistics.num_photons")->second);
	num_scattered = stoll(dict.find("statistics.num_scattered")->second);
	total_time = stod(dict.find("statistics.total_time")->second);
	scatter_time = stod(dict.find("statistics.scatter_time")->second);
	build_time = stod(dict.find("statistics.build_time")->second);
	gather_time = stod(dict.find("statistics.gather_time")->second);
	merge_time = stod(dict.find("statistics.merge_time")->second);
	density_time = stod(dict.find("statistics.density_time")->second);
	intersect_time = stod(dict.find("statistics.intersect_time")->second);
	trace_eye_time = stod(dict.find("statistics.trace_eye_time")->second);
	trace_light_time = stod(dict.find("statistics.trace_light_time")->second);

  const string prefix = "records[";
  const string rms_error = "].rms_error";
  const string abs_error = "].abs_error";
  const string clock_time = "].clock_time";
  const string frame_duration = "].frame_duration";
  const string numeric_errors = "].numeric_errors";

  auto ensure_size = [&](size_t i) {
    if (i >= dict.size()) {
      return false;
    }

    if (records.size() <= i) {
      records.resize(i + 1);
    }

    return true;
  };

  records.resize(num_samples);

  for (auto&& item : dict) {
    unsigned long long i = 0;

    if (startswith(item.first, prefix) &&
      sscanf(item.first.c_str() + prefix.size(), "%llu", &i) == 1) {

      if (endswith(item.first, rms_error) && ensure_size(i)) {
        records[i].rms_error = (float)stod(item.second);
      }
      else if (endswith(item.first, abs_error) && ensure_size(i)) {
        records[i].abs_error = (float)stod(item.second);
      }
      else if (endswith(item.first, clock_time) && ensure_size(i)) {
        records[i].clock_time = (float)stod(item.second);
      }
      else if (endswith(item.first, frame_duration) && ensure_size(i)) {
        records[i].frame_duration = (float)stod(item.second);
      }
      else if (endswith(item.first, numeric_errors) && ensure_size(i)) {
        records[i].numeric_errors = (size_t)stoll(item.second);
      }
    }
  }
}

map<string, string> statistics_t::to_dict() const {
	map<string, string> result;

	result["statistics.num_samples"] = std::to_string(num_samples);
  result["statistics.num_basic_rays"] = std::to_string(num_basic_rays);
  result["statistics.num_shadow_rays"] = std::to_string(num_shadow_rays);
  result["statistics.num_tentative_rays"] = std::to_string(num_tentative_rays);
  result["statistics.num_photons"] = std::to_string(num_photons);
  result["statistics.num_scattered"] = std::to_string(num_scattered);
  result["statistics.total_time"] = std::to_string(total_time);
  result["statistics.scatter_time"] = std::to_string(scatter_time);
  result["statistics.build_time"] = std::to_string(build_time);
  result["statistics.gather_time"] = std::to_string(gather_time);
  result["statistics.merge_time"] = std::to_string(merge_time);
  result["statistics.density_time"] = std::to_string(density_time);
  result["statistics.intersect_time"] = std::to_string(intersect_time);
  result["statistics.trace_eye_time"] = std::to_string(trace_eye_time);
  result["statistics.trace_light_time"] = std::to_string(trace_light_time);

  char buffer[128];

  for (size_t i = 0; i < records.size(); ++i) {
    sprintf(buffer, "records[%llu].rms_error", (unsigned long long)i);
    result[buffer] = std::to_string(records[i].rms_error);
    sprintf(buffer, "records[%llu].abs_error", (unsigned long long)i);
    result[buffer] = std::to_string(records[i].abs_error);
    sprintf(buffer, "records[%llu].clock_time", (unsigned long long)i);
    result[buffer] = std::to_string(records[i].clock_time);
    sprintf(buffer, "records[%llu].frame_duration", (unsigned long long)i);
    result[buffer] = std::to_string(records[i].frame_duration);
    sprintf(buffer, "records[%llu].numeric_errors", (unsigned long long)i);
    result[buffer] = std::to_string(records[i].numeric_errors);
  }

  return result;
}

std::ostream& operator<<(std::ostream& stream, const statistics_t& meta) {
  double connection_time =
      meta.trace_eye_time - meta.trace_light_time - meta.gather_time;
  double query_time = meta.gather_time - meta.merge_time - meta.intersect_time;
  double generate_time = meta.scatter_time - meta.build_time;
  double merge_remaining_time = meta.merge_time - meta.density_time;

  stream << "num basic rays: " << meta.num_basic_rays << "\n"
         << "num shadow rays: " << meta.num_shadow_rays << "\n"
         << "num tentative rays: " << meta.num_tentative_rays << "\n"
         << "num photons: " << meta.num_photons << "\n"
         << "num scattered: " << meta.num_scattered / meta.num_samples << " ("
         << (meta.num_scattered / meta.num_samples + meta.num_photons - 1) /
                std::max(size_t(1), meta.num_photons)
         << "x)\n"
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

}

