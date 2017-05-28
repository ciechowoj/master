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
	size_t num_errors = 0;
	size_t num_samples = 0;

	void add(const counters_t& counters) {
		num_errors += counters.num_errors;
		num_samples += counters.num_samples;
	}

	void reset() {
		num_errors = 0;
		num_samples = 0;
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

struct context_t;

struct subcontext_t {
	size_t width;
	size_t height;
	size_t sample_number;
	size_t camera_id;
	counters_t counters;
	vector<dvec3> camera_image;
	vector<dvec3> light_image;
	function<void(subcontext_t*, const scene_t*)> render;

	subcontext_t(const context_t* context);
};

struct context_t {
	size_t width = 0;
	size_t height = 0;
	size_t num_samples = 0;
	size_t camera_id = 0;
	counters_t counters;
	vector<dvec4> image;
	function<void(subcontext_t*, const scene_t*)> render;
	function<bool(context_t*, const scene_t*)> update;
	function<void(context_t*)> finish;

	context_t() = default;
	context_t(config_t config);
};

void render(context_t* context, threadpool_t* threadpool, const scene_t* scene, size_t num_samples);

}
