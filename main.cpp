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

function<void(subcontext_t*, const scene_t*)> make_renderer(Options options) {
  auto device = std::shared_ptr<__RTCDevice>(rtcNewDevice(NULL), rtcDeleteDevice);
  runtime_assert(device != nullptr);

  options.numThreads = 1;

  auto scene = loadScene(options);
  scene->buildAccelStructs(device.get());

  auto technique = makeTechnique(scene, options);

  auto generator = make_shared<random_generator_t>();

  return [=](subcontext_t* subcontext, const scene_t*) {
    technique->render(subcontext, generator.get());
  };
}

int run_fast(Options options) {
	GLFWwindow* window = options.batch ? nullptr : setup_glfw_window(
		options.width,
		options.height,
		options.caption().c_str());

	{ // scope
		context_t context(options);

    context.render = make_renderer(options);

    auto scale = make_shared<float>(10.0f);
    auto counters = make_shared<counters_t>();

    context.update = [=](context_t* context, const scene_t* scene) -> bool {
      *counters = context->counters;

      if (window) {
        update_glfw_window(window, context->image.data());
        return glfwWindowShouldClose(window);
      }
      else {
        auto elapsed_time = context->counters.elapsed_time;
        auto num_samples = context->counters.num_samples;
        std::cout << "Time per sample: " << elapsed_time / num_samples << std::endl;
        return false;
      }
    };

		context.finish = [=](context_t*) {
      if (window) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
      }
		};

		threadpool_t threadpool;
		render(&context, &threadpool, nullptr, 1024);

    if (window) {
		  run_glfw_window_loop(window, scale, make_interface_updater(scale, counters));
    }
	}

  if (window) {
	  cleanup_glfw_window(window);
  }

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
	else if (options.fast) {
		return run_fast(options);
	}
	else {
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
