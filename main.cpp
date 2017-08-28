#include <xmmintrin.h>
#include <pmmintrin.h>
#include <runtime_assert>

#include <unittest>

#include <Application.hpp>

#include <threadpool.hpp>
#include <iostream>
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

    if (options.action == Options::AVG) {
        vec3 average = exr_average(options.input0);
        std::cout << average.x << " " << average.y << " " << average.z << std::endl;
    }
    else if (options.action == Options::Errors) {
        std::cout << compute_errors(options.input0, options.input1);
    }
    else if (options.action == Options::SUB) {
        subtract_exr(options.output, options.input0, options.input1);
    }
    else if (options.action == Options::Strip) {
        strip_exr(options.output, options.input0);
    }
    else if (options.action == Options::Merge) {
        merge_exr(options.output, options.input0, options.input1);
    }
    else if (options.action == Options::Filter) {
        filter_exr(options.output, options.input0);
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
    else if (options.action == Options::Gnuplot) {
        auto error_message = gnuplot(argc, argv);

        options.displayHelp = !error_message.empty();
        options.displayMessage = error_message;

        auto status = displayHelpIfNecessary(options, "0.0.1");

        if (status.first) {
          return status.second;
        }
    }
    else {
        if (options.action == Options::Continue) {
            map<string, string> metadata = load_metadata(options.input0);
            auto output = options.input0;
            options = Options(metadata);
            options.output = output;
            options.action = Options::Continue;
            options.num_seconds = 0;
            options.num_samples = 0;

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
