#include <algorithm>
#include <unordered_map>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <statistics.hpp>
#include <utility.hpp>

namespace haste {

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

  const string records_prefix = "records[";
  const string rms_error = "].rms_error";
  const string abs_error = "].abs_error";
  const string clock_time = "].clock_time";
  const string frame_duration = "].frame_duration";
  const string numeric_errors = "].numeric_errors";

  const string measurements_prefix = "measurements[";
  const string pixel_x = "].pixel_x";
  const string pixel_y = "].pixel_y";
  const string value = "].pixel_y";

  std::unordered_map<size_t, record_t> records_map;
  std::unordered_map<size_t, measurement_t> measurements_map;

  for (auto&& item : dict) {
    unsigned long long i = 0;

    if (startswith(item.first, records_prefix) &&
      sscanf(item.first.c_str() + records_prefix.size(), "%llu", &i) == 1) {

      size_t index = size_t(i);

      if (endswith(item.first, rms_error)) {
        records_map[index].rms_error = (float)stod(item.second);
      }
      else if (endswith(item.first, abs_error)) {
        records_map[index].abs_error = (float)stod(item.second);
      }
      else if (endswith(item.first, clock_time)) {
        records_map[index].clock_time = (float)stod(item.second);
      }
      else if (endswith(item.first, frame_duration)) {
        records_map[index].frame_duration = (float)stod(item.second);
      }
      else if (endswith(item.first, numeric_errors)) {
        records_map[index].numeric_errors = (size_t)stoll(item.second);
      }
    }
    else if (startswith(item.first, measurements_prefix) &&
      sscanf(item.first.c_str() + measurements_prefix.size(), "%llu", &i) == 1) {

      size_t index = size_t(i);
      float x, y, z;

      if (endswith(item.first, pixel_x)) {
        measurements_map[index].pixel_x = stoi(item.second);
      }
      else if (endswith(item.first, pixel_y)) {
        measurements_map[index].pixel_y = stoi(item.second);
      }
      else if (endswith(item.first, rms_error)) {
        measurements_map[index].rms_error = (float)stod(item.second);
      }
      else if (endswith(item.first, abs_error)) {
        measurements_map[index].abs_error = (float)stod(item.second);
      }
      else if (endswith(item.first, value) && sscanf(item.second.c_str(), "[%f, %f, %f]", &x, &y, &z)) {
        measurements_map[index].value.x = float(x);
        measurements_map[index].value.y = float(y);
        measurements_map[index].value.z = float(z);
      }
    }
  }

  for (auto&& itr : records_map) {
    records.push_back(itr.second);
    records.back().sample_index = itr.first;
  }

  for (auto&& itr : measurements_map) {
    measurements.push_back(itr.second);
    measurements.back().sample_index = itr.first;
  }

  std::sort(records.begin(), records.end(), [](const record_t& a, const record_t& b) {
    return a.sample_index < b.sample_index; });
  std::sort(measurements.begin(), measurements.end(), [](const measurement_t& a, const measurement_t& b) {
    return a.sample_index < b.sample_index; });
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
  char value_buffer[1024];

  for (size_t i = 0; i < records.size(); ++i) {
    size_t sample_index = records[i].sample_index;
    sprintf(buffer, "records[%llu].rms_error", (unsigned long long)sample_index);
    result[buffer] = std::to_string(records[i].rms_error);
    sprintf(buffer, "records[%llu].abs_error", (unsigned long long)sample_index);
    result[buffer] = std::to_string(records[i].abs_error);
    sprintf(buffer, "records[%llu].clock_time", (unsigned long long)sample_index);
    result[buffer] = std::to_string(records[i].clock_time);
    sprintf(buffer, "records[%llu].frame_duration", (unsigned long long)sample_index);
    result[buffer] = std::to_string(records[i].frame_duration);
    sprintf(buffer, "records[%llu].numeric_errors", (unsigned long long)sample_index);
    result[buffer] = std::to_string(records[i].numeric_errors);
  }

  for (size_t i = 0; i < measurements.size(); ++i) {
    size_t sample_index = measurements[i].sample_index;
    sprintf(buffer, "measurements[%llu].pixel_x", (unsigned long long)sample_index);
    result[buffer] = std::to_string(measurements[i].pixel_x);
    sprintf(buffer, "measurements[%llu].pixel_y", (unsigned long long)sample_index);
    result[buffer] = std::to_string(measurements[i].pixel_y);
    sprintf(buffer, "measurements[%llu].rms_error", (unsigned long long)sample_index);
    result[buffer] = std::to_string(measurements[i].rms_error);
    sprintf(buffer, "measurements[%llu].abs_error", (unsigned long long)sample_index);
    result[buffer] = std::to_string(measurements[i].abs_error);
    sprintf(buffer, "measurements[%llu].value", (unsigned long long)sample_index);
    sprintf(value_buffer, "[%f, %f, %f]", measurements[i].value.x, measurements[i].value.y, measurements[i].value.z);
    result[buffer] = value_buffer;
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

void print_frame_summary(std::ostream& stream, const statistics_t& statistics)
{
  if (!statistics.records.empty()) {
    stream << "#" << std::setw(8) << std::left << statistics.num_samples << " ";
    stream << std::right << std::fixed << std::setw(8) << std::setprecision(3) << statistics.total_time << "s";
    stream << std::setw(8) << statistics.records.back().frame_duration << "s/sample   ";
    stream << "rms:abs " << std::setprecision(8) << statistics.records.back().rms_error << ":" << statistics.records.back().abs_error;
    stream << std::endl;
  }
}

void print_records_tabular(std::ostream& stream, const statistics_t& statistics) {
  auto& records = statistics.records;

  if (!records.empty()) {
    auto last_index = std::to_string(records.size() - 1);
    auto last_time = std::to_string(size_t(statistics.total_time));

    double rendering_duration = 0;

    for (size_t i = 0; i < records.size(); ++i) {
      rendering_duration += records[i].frame_duration;

      stream << std::setw(last_index.size()) << std::left << records[i].sample_index;
      stream << std::right << std::setw(last_time.size() + 9) << std::fixed << std::setprecision(7) << rendering_duration;
      stream << std::setw(10) << records[i].rms_error;
      stream << std::setw(10) << records[i].abs_error;
      stream << std::setw(5) << records[i].numeric_errors;
      stream << "\n";
    }

    stream.flush();
  }
}

struct ivec2_less {
    bool operator()(const ivec2& a, const ivec2& b) {
        return a.x == b.x ? a.y < b.y : a.x < b.x;
    };
};

void print_measurements_tabular(std::ostream& stream, const statistics_t& statistics) {
  using measurement_t = statistics_t::measurement_t;

  map<ivec2, vector<measurement_t>, ivec2_less> measurements;

  for (auto&& itr : statistics.measurements) {
    ivec2 position = ivec2(itr.pixel_x, itr.pixel_y);
    measurements[position].push_back(itr);
  }

  if (measurements.empty()) {
    return;
  }

  size_t size = statistics.records.size();

  for (auto&& itr : measurements) {
    size = std::min(size, itr.second.size());
  }

  std::stringstream sstream;

  sstream << std::fixed << std::setprecision(7) << std::setw(12);

  for (auto&& itr : measurements) {
    auto& measurement = itr.second;
    sstream << "# ";
    sstream << measurement[0].pixel_x << "x" << measurement[0].pixel_y << "xRMS ";
    sstream << measurement[0].pixel_x << "x" << measurement[0].pixel_y << "xABS ";
  }

  sstream << "\n";

  double total_time = 0.0f;

  for (size_t i = 0; i < size; ++i) {

    sstream << std::setw(12) << total_time << " ";

    for (auto&& itr : measurements) {
      auto& measurement = itr.second;
      sstream << std::setw(12) << measurement[i].rms_error << " "
              << std::setw(12) << measurement[i].abs_error << " ";
    }

    sstream << "\n";

    total_time += statistics.records[i].frame_duration;
  }

  stream << sstream.str();

}
}
