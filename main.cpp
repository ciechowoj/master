#include <xmmintrin.h>
#include <pmmintrin.h>
#include <runtime_assert>

#include <unittest>

#include <Application.hpp>

#include <make_technique.hpp>
#include <threadpool.hpp>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <atomic>
#include <set>
#include <mutex>

#include <exr.hpp>
#include <gnuplot.hpp>

using namespace std;
using namespace haste;

unittest() {
  #if defined _MSC_VER
	#pragma warning( disable : 4723)
  #endif

    // check if everything is configured well
    assert_almost_eq(sin(half_pi<float>()), 1.0f);
    assert_almost_eq(asin(1.0f), half_pi<float>());

    #if !defined _MSC_VER
    assert_true(std::isinf(1.0f / 0.0f));
    assert_false(std::isnan(1.0f / 1.0f));
    assert_true(std::isnan(0.0f / 0.0f));
    assert_true(std::isnan(-0.0f / 0.0f));
    #endif
}

unittest() {
    // default constructed matrix is identity matrix
    assert_almost_eq(mat4() * vec4(1.0f, 2.0f, 3.0f, 4.0f), vec4(1.0f, 2.0f, 3.0f, 4.0f));

    // matrix indexing is column major
    mat4 m;
    m[0] = vec4(1.0f, 1.0f, 0.0f, 0.0f);

    // 1 0 0 0
    // 1 1 0 0
    // 0 0 1 0
    // 0 0 0 1
    // ^

    assert_almost_eq(m * vec4(1.0f), vec4(1.0f, 2.0f, 1.0f, 1.0f));
}

int run_fast(Options options) {
    return 0;
}

int bake(int argc, char **argv);
int compute_errors(std::ostream& stream, const Options& options);
int compute_relative_error(const Options& options);

int main(int argc, char **argv) {
    if (!run_all_tests())
        return 1;

    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    Options options = parseArgs(argc, argv);
    auto status = displayHelpIfNecessary(options, "0.0.1");

    if (status.first) {
        return status.second;
    }

    if (options.action == Options::Average) {
        vec3 average = exr_average(options.input0);
        std::cout << "[" << average.x << " " << average.y << " " << average.z << "]" << std::endl;
    }
    else if (options.action == Options::Errors) {
        return compute_errors(std::cout, options);
    }
    else if (options.action == Options::Strip) {
        strip_exr(options.output, options.input0);
    }
    else if (options.action == Options::Merge) {
        merge_exr(options.output, options.input0, options.input1);
    }
    else if (options.action == Options::Time) {
        std::cout << query_time(options.input0);
    }
    else if (options.action == Options::Statistics) {
      auto metadata = load_metadata(options.input0);
      print_records_tabular(std::cout, statistics_t(metadata));
    }
    else if (options.action == Options::Measurements) {
      auto metadata = load_metadata(options.input0);
      print_measurements_tabular(std::cout, statistics_t(metadata));
    }
    else if (options.action == Options::Traces) {
      auto metadata = load_metadata(options.input0);
      print_traces_tabular(std::cout, metadata);
    }
    else if (options.action == Options::Gnuplot) {
        auto error_message = gnuplot(argc, argv);

        options.displayHelp = !error_message.empty();
        options.displayMessage = error_message;

        auto status = displayHelpIfNecessary(options, "0.0.1");

        if (status.first) {
          return status.second;
        }
    }
    else if (options.action == Options::Bake) {
        return bake(argc, argv);
    }
    else if (options.action == Options::RelErr) {
        return compute_relative_error(options);
    }
    else {
        if (options.action == Options::Continue) {
            map<string, string> metadata = load_metadata(options.input0);
            auto output = options.input0;
            options = Options(metadata);
            options.output = output;
            options.action = Options::Continue;
            options.num_samples = atoi(metadata["statistics.num_samples"].c_str());
            options.num_seconds = atoi(metadata["statistics.total_time"].c_str());
            overrideArgs(options, argc, argv);

            auto status = displayHelpIfNecessary(options, "0.0.1");

            if (status.first) {
                return status.second;
            }
        }

        Application application(options);

        if (options.batch) {
            return application.runBatch(options.width, options.height);
        }
        else {
            return application.run(options.width, options.height, options.caption());
        }
    }

    return 0;
}

int bake(int argc, char **argv) {
  auto options = extractOptions(argc - 1, argv + 1);

  if (displayHelp(options)) {
    return 0;
  }

  auto itr = options.find("--input");

  if (itr == options.end()) {
    std::cout << "The input file is required." << std::endl;
    displayHelp();
    return 1;
  }

  string input = itr->second;
  options.erase(itr);

  itr = options.find("--output");

  if (itr == options.end()) {
    std::cout << "The output file is required." << std::endl;
    displayHelp();
    return 1;
  }

  string output = itr->second;
  options.erase(itr);

  if (options.size() != 0) {
    std::cout
        << "Unexpected option '"
        << options.begin()->first
        << "' "
        << options.begin()->second
        << ".\n";

    displayHelp();
    return 1;
  }

  map<string, string> metadata;
  size_t width, height;
  vector<vec4> data4;
  vector<vec3> data3;

  load_exr(input, metadata, width, height, data4);

  data3.resize(data4.size());

  for (size_t i = 0; i < data4.size(); ++i) {
    data3[i] = data4[i].xyz() / data4[i].w;
  }

  save_exr(output, metadata, width, height, data3);

  return 0;
}

int compute_errors(std::ostream& stream, const Options& options) {
  vector<vec3> fst_data, snd_data;
  map<string, string> fst_metadata, snd_metadata;

  size_t fst_width = 0, snd_width = 0, fst_height = 0, snd_height = 0;

  load_exr(options.input0, fst_metadata, fst_width, fst_height, fst_data);
  load_exr(options.input1, snd_metadata, snd_width, snd_height, snd_data);

  if (fst_width != snd_width || fst_height != snd_height) {
    throw std::runtime_error("Sizes of '" + options.input0 +
      "' and '" + options.input1 + "' doesn't match.");
  }

  double num = 0.0f;
  double abs_sum = 0.0;
  double rms_sum = 0.0;

  for (size_t i = 0; i < fst_data.size(); ++i) {
    if (!any(isnan(fst_data[i])) && !any(isnan(snd_data[i]))) {
      auto diff = fst_data[i] - snd_data[i];
      rms_sum += l1Norm(diff * diff);
      abs_sum += l1Norm(diff);
      num += 3.0f;
    }
  }

  for (size_t j = 0; j < options.trace.size(); ++j) {
    double num = 0.0f;
    double abs_sum = 0.0;
    double rms_sum = 0.0;

    int x0 = std::max(0, options.trace[j].x - options.trace[j].z);
    int y0 = std::max(0, options.trace[j].y - options.trace[j].z);
    int x1 = std::min(int(fst_width), options.trace[j].x + options.trace[j].z);
    int y1 = std::min(int(fst_height), options.trace[j].y + options.trace[j].z);

    for (int y = y0; y < y1; ++y) {
      for (int x = x0; x < x1; ++x) {
        int i = y * fst_width + x;
        if (!any(isnan(fst_data[i])) && !any(isnan(snd_data[i]))) {
          auto diff = fst_data[i] - snd_data[i];
          rms_sum += l1Norm(diff * diff);
          abs_sum += l1Norm(diff);
          num += 3.0f;
        }
      }
    }

    stream << std::setprecision(10)
      << options.trace[j].x << "x" << options.trace[j].y << "x" << options.trace[j].z << " "
      << abs_sum / num << " "
      << sqrt(rms_sum / num) << "\n";
  }

  stream << std::setprecision(10)
    << "ABS: " << abs_sum / num << "\n"
    << "RMS: " << sqrt(rms_sum / num) << "\n";

  return 0;
}

int compute_relative_error(const Options& options) {
  vector<vec3> diff_data, fst_data, snd_data, blur_data;
  map<string, string> fst_metadata, snd_metadata;

  size_t fst_width = 0, snd_width = 0, fst_height = 0, snd_height = 0;

  load_exr(options.input0, fst_metadata, fst_width, fst_height, fst_data);
  load_exr(options.input1, snd_metadata, snd_width, snd_height, snd_data);

  if (fst_width != snd_width || fst_height != snd_height) {
    throw std::runtime_error("Sizes of '" + options.input0 +
      "' and '" + options.input1 + "' doesn't match.");
  }

  // compute difference
  diff_data.resize(fst_width * fst_height);

  for (size_t i = 0; i < fst_data.size(); ++i) {
    if (!any(isnan(fst_data[i])) && !any(isnan(snd_data[i]))) {
      diff_data[i] = vec3(length(fst_data[i]) - length(snd_data[i])) / std::min(length(fst_data[i]), length(snd_data[i]));
    }
    else {
      diff_data[i] = vec3(0.f);
    }
  }

  // blur
  blur_data = diff_data;

  ptrdiff_t width = ptrdiff_t(fst_width);
  ptrdiff_t height = ptrdiff_t(fst_height);
  ptrdiff_t window = 2;

  float max;
  if (window > 0) {
    for (int i = 0; i < 3; ++i) {
      max = 0.0f;

      for (ptrdiff_t y = 0; y < height; ++y) {
        for (ptrdiff_t x = 0; x < width; ++x) {
          float num = 0.0f;
          vec3 acc = vec3(0.0f);

          for (ptrdiff_t y_w = std::max<ptrdiff_t>(0, y - window); y_w < std::min<ptrdiff_t>(height, y + window + 1); ++y_w) {
            for (ptrdiff_t x_w = std::max<ptrdiff_t>(0, x - window); x_w < std::min<ptrdiff_t>(width, x + window + 1); ++x_w) {
              acc += diff_data[y_w * width + x_w];
              num += 1.0f;
            }
          }

          blur_data[y * width + x] = acc / num;
          max = std::max(max, abs(blur_data[y * width + x].x));
        }
      }

      diff_data = blur_data;
    }
  }
  std::cout << "max: " << max << std::endl;

  // draw gauge
  float x0 = 0.9, y0 = 0.05, x1 = 0.95, y1 = 0.95;

  ptrdiff_t x0i = ptrdiff_t(x0 * width);
  ptrdiff_t y0i = ptrdiff_t(y0 * height);
  ptrdiff_t x1i = ptrdiff_t(x1 * width);
  ptrdiff_t y1i = ptrdiff_t(y1 * height);

  max = 1;
  float range = 2 * max;

  for (auto y = 0; y < (y1i - y0i); ++y) {
    for (auto x = 0; x < (x1i - x0i); ++x) {

      float i = range * float(y) / float(y1i - y0i) - max;

      blur_data[(y + y0i) * width + x + x0i] = vec3(i, i, i);
    }
  }

  // map to color
  for (ptrdiff_t y = 0; y < height; ++y) {
    for (ptrdiff_t x = 0; x < width; ++x) {
      auto p = blur_data[y * width + x];

      blur_data[y * width + x] = p.x < 0.0f
      ? vec3(0.0f, std::max(0.f, -p.x * 2), std::max(0.f, -p.x * 2 - 1.f))
      : vec3(std::max(0.f, p.x * 2.f), std::max(0.f, p.x * 2 - 1.f), 0.f);
    }
  }

  save_exr(options.output, fst_metadata, width, height, blur_data);

  return 0;
}



