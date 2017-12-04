#include <stdexcept>
#include <gnuplot.hpp>
#include <utility.hpp>
#include <exr.hpp>
#include <statistics.hpp>
#include <system_utils.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <array>
#include <cstring>

namespace haste {

struct ivec2_less {
    bool operator()(const ivec2& a, const ivec2& b) const {
        return a.x == b.x ? a.y < b.y : a.x < b.x;
    };
};

statistics_t load_statistics(string path) {
  map<string, string> metadata = load_metadata(path);
  return statistics_t(metadata);
}

std::string exec(string cmd) {
  std::array<char, 128> buffer;
  std::string result;
  #if defined _MSC_VER
  std::shared_ptr<FILE> pipe(_popen(cmd.c_str(), "r"), _pclose);
  #else
  std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
  #endif
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
    if (fgets(buffer.data(), 128, pipe.get()) != NULL)
      result += buffer.data();
  }
  return result;
}

struct series_t {
  string label;
  ivec2 point;
  vector<vec2> values;
};

struct dataset_t {
  vector<series_t> series;
};

dataset_t make_dataset(const vector<string>& inputs) {
  dataset_t dataset;

  for (auto&& input : inputs) {
    statistics_t statistics = load_statistics(input);

    vector<float> frame_times = vector<float>(statistics.records.size() + 1);
    frame_times[0] = 0.0f;

    for (size_t i = 0; i < statistics.records.size(); ++i) {
      frame_times[i + 1] = frame_times[i] + statistics.records[i].frame_duration;
    }

    struct series_info_t {
      size_t index;
      size_t size;
      string label;
      ivec2 point;
    };

    map<ivec2, series_info_t, ivec2_less> series_info;

    for (auto&& measurement : statistics.measurements) {
      ivec2 key = ivec2(measurement.pixel_x, measurement.pixel_y);

      auto itr = series_info.find(key);

      if (itr != series_info.end()) {
        itr->second.size = std::max(itr->second.size, measurement.sample_index + 1);
      }
      else {
        series_info_t info;
        info.index = series_info.size() * 2 + dataset.series.size();
        info.size = measurement.sample_index + 1;
        info.label = input + " " + std::to_string(key.x) + "x" + std::to_string(key.y);
        info.point = key;
        series_info.insert(std::make_pair(key, info));
      }
    }

    dataset.series.resize(dataset.series.size() + series_info.size() * 2);

    for (auto&& info_itr : series_info) {
      auto info = info_itr.second;
      auto size = std::min(info.size, statistics.records.size());
      dataset.series[info.index + 0].label = info.label + " RMS";
      dataset.series[info.index + 0].point = info.point;
      dataset.series[info.index + 0].values.resize(size);
      dataset.series[info.index + 1].label = info.label + " ABS";
      dataset.series[info.index + 1].point = info.point;
      dataset.series[info.index + 1].values.resize(size);
    }

    for (auto&& measurement : statistics.measurements) {
      ivec2 key = ivec2(measurement.pixel_x, measurement.pixel_y);
      auto info = series_info.find(key)->second;

      if (measurement.sample_index < info.size) {
        dataset.series[info.index + 0].values[measurement.sample_index].x = frame_times[measurement.sample_index];
        dataset.series[info.index + 0].values[measurement.sample_index].y = measurement.rms_error;
        dataset.series[info.index + 1].values[measurement.sample_index].x = frame_times[measurement.sample_index];
        dataset.series[info.index + 1].values[measurement.sample_index].y = measurement.abs_error;
      }
    }
  }

  return dataset;
}

// https://stackoverflow.com/questions/3300419/file-name-matching-with-wildcard
bool match(char const *needle, char const *haystack) {
    for (; *needle != '\0'; ++needle) {
        switch (*needle) {
        case '?':
            if (*haystack == '\0')
                return false;
            ++haystack;
            break;
        case '*': {
            if (needle[1] == '\0')
                return true;
            size_t max = std::strlen(haystack);
            for (size_t i = 0; i < max; i++)
                if (match(needle + 1, haystack + i))
                    return true;
            return false;
        }
        default:
            if (*haystack != *needle)
                return false;
            ++haystack;
        }
    }
    return *haystack == '\0';
}

dataset_t select_series(dataset_t&& dataset, const vector<string>& selections) {
  if (selections.empty()) {
    return std::move(dataset);
  }
  else {
    dataset_t result;

    for (auto&& series : dataset.series) {
      for (auto&& selection : selections) {
        if (match(selection.c_str(), series.label.c_str())) {
          result.series.push_back(std::move(series));
          break;
        }
      }
    }

    return result;
  }
}

void make_gnuplot_script(
  std::ostream& stream,
  string output,
  const dataset_t& dataset,
  const vector<string>& temp_series) {

  stream << "set terminal pngcairo enhanced truecolor size 1024,1024" << "\n";
  stream << "set logscale y\n";
  stream << "set output '" << output << "'\n\n";

  for (size_t i = 0; i < temp_series.size(); ++i) {
    stream << (i == 0 ? "plot " : "     ");
    stream << "'" << temp_series[i] << "' using 1:2 with lines " << "title '" << dataset.series[i].label << "'";
    stream << (i == temp_series.size() - 1 ? "\n" : ", \\\n");
  }
}

void make_gnuplot_script_grouped(
  std::ostream& stream,
  string output,
  const dataset_t& dataset,
  const vector<string>& temp_series) {

  struct series_ref_t
  {
    const series_t* series;
    string path;
  };

  map<ivec2, vector<series_ref_t>, ivec2_less> groups;

  for (size_t i = 0; i < temp_series.size(); ++i) {
    series_ref_t ref;
    ref.series = &dataset.series[i];
    ref.path = temp_series[i];

    auto itr = groups.find(ref.series->point);
    if (itr != groups.end()) {
      itr->second.push_back(ref);
    }
    else {
      groups.insert(std::make_pair(ref.series->point, vector<series_ref_t>(1, ref)));
    }
  }

  int height = int(sqrt(groups.size()));
  int width = (groups.size() + height - 1) / height;

  stream << "set terminal pngcairo enhanced truecolor "
         << "size " << 1024 * width << "," << 1024 * height << "\n";
  stream << "set logscale y\n";
  stream << "set output '" << output << "'\n\n";

  stream << "set multiplot layout " << height << "," << width << "\n";

  for (auto&& group : groups) {
    for (auto itr = group.second.begin(); itr != group.second.end(); ++itr) {
      stream << (itr == group.second.begin() ? "plot " : "     ");
      stream
        << "'" << itr->path << "' using 1:2 with lines "
        << "title '" << itr->series->label << "'";
      stream << (itr + 1 == group.second.end() ? "\n" : ", \\\n");
    }
  }
}

string gnuplot(string output, const dataset_t& dataset) {
  vector<string> temp_series;
  string temp_script = temppath(".gnuplot.txt");

  for (size_t i = 0; i < dataset.series.size(); ++i) {
    temp_series.push_back(temppath(".txt"));
  }

  for (size_t i = 0; i < temp_series.size(); ++i) {
    std::ofstream stream(temp_series[i], std::ios::out | std::ios::trunc);

    stream << "# " << dataset.series[i].label << "\n";

    for (auto&& value : dataset.series[i].values) {
      stream << value.x << " " << value.y << "\n";
    }
  }

  std::ofstream stream(temp_script, std::ios::out | std::ios::trunc);
  make_gnuplot_script_grouped(stream, output, dataset, temp_series);
  stream.close();

  auto result = exec("gnuplot " + temp_script);
  std::cout << result;

  for (auto&& temp : temp_series) {
    std::remove(temp.c_str());
  }

  std::remove(temp_script.c_str());

  return string();
}

string handle_output(std::multimap<string, string>& options, string& output) {
  if (options.count("--output") == 0) {
    return "Output file is required.";
  }

  if (options.count("--output") != 1) {
    return "There must be only one output file.";
  }

  auto itr = options.find("--output");
  output = itr->second;
  options.erase(itr);

  return string();
}

string handle_error(std::multimap<string, string>& options, string& error) {
  auto count = options.count("--error");

  if (count == 0) {
    return string();
  }

  if (count > 1) {
    return "There must not be multiple error types specified.";
  }

  auto itr = options.find("--error");

  if (itr->second != "rms" && itr->second != "abs") {
    return "Unsupported error type " + string(itr->second) + ".";
  }

  error = itr->second;
  options.erase(itr);

  return string();
}

string handle_switch(std::multimap<string, string>& options, string switch_, bool& value) {
  auto count = options.count(switch_);

  if (count == 0) {
    value = false;
    return string();
  }

  for (auto itr = options.find(switch_); itr != options.end(); itr = options.find(switch_)) {
    options.erase(switch_);
  }

  value = true;

  return string();
}

string handle_select(std::multimap<string, string>& options, vector<string>& selections) {
  auto count = options.count("--select");

  selections.clear();

  if (count == 0) {
    return string();
  }

  for (auto itr = options.find("--select"); itr != options.end(); itr = options.find("--select")) {
    selections.push_back(itr->second);
    options.erase("--select");
  }

  return string();
}

string gnuplot_times(int argc, char const* const* argv);

struct gnuplot_arguments {
  vector<string> inputs;
  string output;
  string mode;
  bool traces;

  string to_string() const {
    string inputs = "[";

    if (!this->inputs.empty()) {
      inputs += "\"" + this->inputs[0] + "\"";
    }

    for (size_t i = 1; i < this->inputs.size(); ++i) {
      inputs += ", \"" + this->inputs[i] + "\"";
    }

    inputs += "]";


    return "{ \"inputs\": " + inputs + ", " +
      "\"output\": \"" + output + "\", " +
      "\"mode\": \"" + mode + "\", " +
      "\"traces\": " + (traces ? string("true") : string("false")) + " }";
  }
};

gnuplot_arguments parse_arguments(int argc, char const* const* argv);

string gnuplot(int argc, char const* const* argv) {
  if (argc < 2) {
    throw std::invalid_argument("argc");
  }

  if (argv[1] != string("gnuplot")) {
    throw std::invalid_argument("argv");
  }

  if (argc > 2 && argv[2] == string("times")) {
    return gnuplot_times(argc, argv);
  }

  auto options = extract_options(argc - 2, argv + 2);

  string output;
  string error_message = handle_output(options, output);

  if (!error_message.empty()) {
    return error_message;
  }

  vector<string> inputs;

  auto itr = options.begin();
  while (itr != options.end()) {
    if (itr->first == "--input") {
      inputs.push_back(itr->second);
      itr = options.erase(itr);
    }
    else {
      ++itr;
    }
  }

  string error = "rms";
  error_message = handle_error(options, error);

  if (!error_message.empty()) {
    return error_message;
  }

  bool traces = false;
  error_message = handle_switch(options, "--traces", traces);

  if (!error_message.empty()) {
    return error_message;
  }

  vector<string> selections;
  error_message = handle_select(options, selections);

  if (!error_message.empty()) {
    return error_message;
  }

  if (!options.empty()) {
    return "Unsupported switch " + options.begin()->first + ".";
  }

  return gnuplot(output, select_series(make_dataset(inputs), selections));
}

gnuplot_arguments parse_arguments(int argc, char const* const* argv) {
  if (argc < 2) {
    throw std::invalid_argument("argc");
  }

  gnuplot_arguments result;

  if (argv[1] != string("gnuplot")) {
    throw std::invalid_argument("argv");
  }

  if (argc > 2 && argv[2] == string("times")) {
    result.mode = argv[2];
  }

  auto options = extract_options(argc - 3, argv + 3);

  string error_message = handle_output(options, result.output);

  if (!error_message.empty()) {
    throw std::invalid_argument(error_message);
  }

  auto itr = options.begin();
  while (itr != options.end()) {
    if (itr->first == "--input") {
      result.inputs.push_back(itr->second);
      itr = options.erase(itr);
    }
    else {
      ++itr;
    }
  }

  error_message = handle_switch(options, "--traces", result.traces);

  if (!error_message.empty()) {
    throw std::invalid_argument(error_message);
  }

  return result;
}

struct time_series_t {
  struct value_t {
    size_t frame;
    float time;
  };

  string label;
  vector<value_t> values;
};

vector<time_series_t> make_time_series(const vector<string>& inputs) {
  vector<time_series_t> result;

  for (auto&& input : inputs) {
    auto statistics = load_statistics(input);

    time_series_t series;
    series.label = input;
    series.values.resize(statistics.records.size());

    for (size_t i = 0; i < series.values.size(); ++i) {
      series.values[i].frame = statistics.records[i].sample_index;
      series.values[i].time = statistics.records[i].frame_duration;
    }

    result.push_back(series);
  }

  return result;
}

string gnuplot_times(int argc, char const* const* argv) {
  auto arguments = parse_arguments(argc, argv);

  string gnuplot_script = temppath(".gnuplot.txt");
  vector<string> source_files = temppaths(arguments.inputs.size(), ".txt");
  vector<time_series_t> series = make_time_series(arguments.inputs);

  for (size_t i = 0; i < source_files.size(); ++i) {
    std::ofstream stream(source_files[i], std::ios::out | std::ios::trunc);
    stream << "# " << series[i].label << "\n";

    for (auto&& value : series[i].values) {
      stream << value.frame << " " << value.time << "\n";
    }
  }

  std::ofstream stream(gnuplot_script, std::ios::out | std::ios::trunc);

  stream << "set terminal pngcairo enhanced truecolor size 4096,1024" << "\n";
  stream << "set yrange [0:]" << "\n";
  stream << "set output '" << arguments.output << "'\n\n";

  for (size_t i = 0; i < source_files.size(); ++i) {
    stream << (i == 0 ? "plot " : "     ");
    stream << "'" << source_files[i] << "' using 1:2 with lines " << "title '" << series[i].label << "'";
    stream << (i == source_files.size() - 1 ? "\n" : ", \\\n");
  }

  stream.close();

  auto result = exec("gnuplot " + gnuplot_script);
  std::cout << result;

  for (auto&& source_file : source_files) {
    std::remove(source_file.c_str());
  }

  std::remove(gnuplot_script.c_str());

  return string();
}

}
