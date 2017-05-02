#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>
#include <vector>
#include <mutex>
#include <threadpool.hpp>
#include <glm>
#include <Options.hpp>
#include <functional>

namespace haste {

using std::vector;
using std::mutex;
using std::function;
using namespace glm;

struct counters_t {
  size_t num_errors;

  void add(const counters_t& counters) {
    num_errors += counters.num_errors;
  }

  void reset() {
    num_errors = 0;
  }
};

struct camera_t {
  int material_id;
  vec3 position;
  vec3 direction;
  vec3 up;
  float fovx;
  float fovy;
  float znear;
  float zfar;
};

struct material_t {
  vec3 emmisive;
  vec3 diffuse;
  vec3 specular;
  float power;
  float index;
};

struct model_t {
  int material_id;
  vector<int> indices;
  vector<vec3> vertices;
  vector<mat3> normals;
};

struct scene_t {
  vector<camera_t> cameras;
  vector<material_t> materials;
  vector<model_t> models;
  RTCScene rtc_scene;

};

struct subcontext_t {
  size_t width;
  size_t height;
  size_t sample_number;
  size_t camera_id;
  counters_t counters;
  vector<dvec3> camera_image;
  vector<dvec3> light_image;
  function<void(subcontext_t*, const scene_t*)> render;
};

struct context_t {
  size_t width;
  size_t height;
  size_t num_samples;
  size_t camera_id;
  counters_t counters;
  vector<dvec4> image;
  function<void(subcontext_t*, const scene_t*)> render;

  context_t(config_t config);
};



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

void render(context_t* context, const scene_t* scene, size_t num_samples) {
  threadpool_t threadpool;

  size_t num_threads = threadpool.num_threads();

  size_t thread_num_samples = (num_samples + num_threads - 1) / num_threads;
  size_t left_num_samples = num_samples;

  size_t samples_started = 0;
  std::mutex samples_mutex;

  for (size_t i = 0; i < threadpool.num_threads(); ++i) {
    size_t local_num_samples = std::min(left_num_samples, thread_num_samples);
    left_num_samples -= thread_num_samples;

    threadpool.exec([&]() {
      subcontext_t subcontext;

      for (size_t j = 0; j < local_num_samples; ++j) {
        {
          std::unique_lock<std::mutex> lock(mutex);
          subcontext.sample_number = ++samples_started;
        }

        subcontext.render(&subcontext, scene);

        {
          std::unique_lock<std::mutex> lock(mutex);
          commit_image(context, &subcontext);
        }
      }
    });
  }
}










}
