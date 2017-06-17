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

namespace haste {

statistics_t load_statistics(string path) {
  map<string, string> metadata;
  load_exr(path, metadata);
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

void make_gnuplot_script(
  std::ostream& stream,
  string output,
  const vector<string>& inputs,
  const vector<string>& temps) {

  stream << "set terminal png enhanced truecolor" << "\n";
  stream << "set output '" << output << "'\n\n";

  for (size_t i = 0; i < inputs.size(); ++i) {
    stream << (i == 0 ? "plot " : "     ");
    stream << "'" << temps[i] << "' using 1:2 with lines " << "title '" << inputs[i] << "'";
    stream << (i == inputs.size() - 1 ? "\n" : ", \\\n");
  }
}

string gnuplot(string output, const vector<string>& inputs, string error) {
  vector<statistics_t> statistics;
  vector<string> temps;
  string temp_script = temppath(".gnuplot.txt");

  for (auto&& input : inputs) {
    statistics.push_back(load_statistics(input));
    temps.push_back(temppath(".txt"));
  }

  for (size_t i = 0; i < temps.size(); ++i) {
    std::ofstream stream(temps[i], std::ios::out | std::ios::trunc);

    double time = 0.f;

    if (error == "rms") {
      stream << "# rms error for " << fullpath(inputs[i]) << "\n";

      for (auto&& record : statistics[i].records) {
        time += record.frame_duration;
        stream << time << " " << record.rms_error << "\n";
      }
    }
    else {
      stream << "# abs error for " << fullpath(inputs[i]) << "\n";

      for (auto&& record : statistics[i].records) {
        time += record.frame_duration;
        stream << time << " " << record.abs_error << "\n";
      }
    }

    std::cout << temps[i] << std::endl;
  }

  std::ofstream stream(temp_script, std::ios::out | std::ios::trunc);
  make_gnuplot_script(stream, output, inputs, temps);
  stream.close();

  auto result = exec("gnuplot " + temp_script);
  std::cout << result;

  for (auto&& temp : temps) {
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

string gnuplot(int argc, char const* const* argv) {
  if (argc < 2) {
    throw std::invalid_argument("argc");
  }

  if (argv[1] != string("gnuplot")) {
    throw std::invalid_argument("argv");
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

  if (!options.empty()) {
    return "Unsupported switch " + options.begin()->first + ".";
  }

  return gnuplot(output, inputs, error);
}

}
