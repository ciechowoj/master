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

using namespace std;
using namespace haste;

unittest() {
    // check if everything is configured well
    assert_almost_eq(sin(half_pi<float>()), 1.0f);
    assert_almost_eq(asin(1.0f), half_pi<float>());

    assert_true(std::isinf(1.0f / 0.0f));
    assert_false(std::isnan(1.0f / 1.0f));
    assert_true(std::isnan(0.0f / 0.0f));
    assert_true(std::isnan(-0.0f / 0.0f));
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
        printAVG(options.input0);
    }
    else if (options.action == Options::Errors) {
        print_errors(options.input0, options.input1);
    }
    else if (options.action == Options::SUB) {
        subtract(options.output, options.input0, options.input1);
    }
    else if (options.action == Options::Merge) {
        merge(options.output, options.input0, options.input1);
    }
    else if (options.action == Options::Filter) {
        filter_out_nan(options.output, options.input0);
    }
    else if (options.action == Options::Time) {
        print_time(options.input0);
    }
    else {
        Application application(options);

        switch (options.batch) {
            case Options::Interactive:
                return application.run(options.width, options.height, options.caption());
            case Options::Batch:
                return application.runBatch(options.width, options.height);
            case Options::Fast:
                return run_fast(options);
        }
    }

    return 0;
}
