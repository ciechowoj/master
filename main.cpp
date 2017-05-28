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

#include <haste.hpp>

using namespace std;
using namespace haste;

unittest() {
	#pragma warning( disable : 4723)

    // check if everything is configured well
    assert_almost_eq(sin(half_pi<float>()), 1.0f);
    assert_almost_eq(asin(1.0f), half_pi<float>());

    // assert_true(std::isinf(1.0f / 0.0f));
    // assert_false(std::isnan(1.0f / 1.0f));
    // assert_true(std::isnan(0.0f / 0.0f));
    // assert_true(std::isnan(-0.0f / 0.0f));
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
	GLFWwindow* window = setup_glfw_window(
		options.width,
		options.height,
		options.caption().c_str());

	{
		context_t context(options);

    context.render = [=](subcontext_t* subcontext, const scene_t* scene) {
      auto& image = subcontext->camera_image;
      fill(image.begin(), image.end(), dvec3(1.0, 0.0, 0.0));
    };

    context.update = [=](context_t* context, const scene_t* scene) {
      update_glfw_window(window, context->image.data());
      return glfwWindowShouldClose(window);
    };

		context.finish = [=](context_t*) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
		};

		threadpool_t threadpool;
		render(&context, &threadpool, nullptr, 1024);
		run_glfw_window_loop(window);
	}

	cleanup_glfw_window(window);

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
	else if (options.batch == Options::Fast) {
		return run_fast(options);
	}
	else {
        Application application(options);

        switch (options.batch) {
            case Options::Interactive:
                return application.run(options.width, options.height, options.caption());
            case Options::Batch:
                return application.runBatch(options.width, options.height);
        }
    }

    return 0;
}
