#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>
#include <vector>
#include <mutex>
#include <threadpool.hpp>
#include <glm>
#include <Options.hpp>
#include <functional>
#include <algorithm>
#include <iostream>
#include <haste.hpp>
#include <utility.hpp>

namespace haste {

subcontext_t::subcontext_t(const context_t* context) {
	this->width = context->width;
	this->height = context->height;
	this->sample_number = 0;
	this->camera_id = context->camera_id;
	this->counters = counters_t();
	this->camera_image = vector<dvec3>(context->image.size(), dvec3(0.0));
	this->light_image = vector<dvec3>(context->image.size(), dvec3(0.0));
	this->render = context->render;
}

context_t::context_t(config_t config) {
	this->width = config.width;
	this->height = config.height;
	this->num_samples = config.numSamples;
	this->camera_id = config.cameraId;
	this->counters = counters_t();
	this->image = vector<dvec4>(this->width * this->height, dvec4(0.0));

	this->render = [](subcontext_t* subcontext, const scene_t*) { };
	this->update = [](context_t* context, const scene_t*) { return false; };
	this->finish = [](context_t*) {	};
}

void commit_image(context_t* context, subcontext_t* subcontext) {
  for (size_t i = 0; i < context->image.size(); ++i) {
    dvec3 value = subcontext->camera_image[i] + subcontext->light_image[i];

    if (std::isfinite(l1Norm(value))) {
      context->image[i] += dvec4(value, 1.0);
    }
    else {
      ++context->counters.num_errors;
    }

    subcontext->camera_image[i] = dvec3(0.f);
    subcontext->light_image[i] = dvec3(0.f);
  }

  ++context->num_samples;
  context->counters.add(subcontext->counters);
  subcontext->counters.reset();
}

void render(context_t* context, threadpool_t* threadpool, const scene_t* scene, size_t num_samples) {
  size_t num_threads = threadpool->num_threads();

  size_t thread_num_samples = (num_samples + num_threads - 1) / num_threads;
  size_t left_num_samples = num_samples;

  auto samples_started = std::make_shared<size_t>();
  auto threads_finished = std::make_shared<size_t>();
  auto samples_mutex = std::make_shared<std::mutex>();

  context->counters.start_time = high_resolution_time();

  for (size_t i = 0; i < num_threads; ++i) {
    size_t local_num_samples = std::min(left_num_samples, thread_num_samples);
    left_num_samples -= thread_num_samples;

    threadpool->exec([=]() {
	    auto subcontext = subcontext_t(context);

	    for (size_t j = 0; j < local_num_samples; ++j) {
		    { // scope
	          auto lock = std::unique_lock<std::mutex>(*samples_mutex);
		      subcontext.sample_number = ++(*samples_started);
		    }

		    subcontext.render(&subcontext, scene);
		    ++subcontext.counters.num_samples;

		    { // scope
		      auto lock = std::unique_lock<std::mutex>(*samples_mutex);
		      commit_image(context, &subcontext);
          context->counters.elapsed_time = high_resolution_time() - context->counters.start_time;
          if (context->update(context, scene)) {
            break;
          }
		    }
      }

	    auto lock = std::unique_lock<std::mutex>(*samples_mutex);
	    if (++(*threads_finished) == num_threads) {
		    context->finish(context);
	    }
    });
  }
}

}
