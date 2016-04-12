#include <GLFW/glfw3.h>
#include <PathTracing.hpp>

namespace haste {

vec3 sampleLight(
    const Scene& scene,
    const vec3& position,
    const vec3& normal,
    const vec3& reflected,
    const mat3& worldToLight,
    const BSDF& bsdf)
{
    auto light = scene.lights.sample(position);

    if (light.radiance != vec3(0.0f)) {
        vec3 incident = light.position - position;
        float distance = length(incident);
        incident = normalize(incident);
        float sqDistanceInv = 1.f / (distance * distance);

        vec3 throughput = bsdf.eval(
            worldToLight * incident,
            worldToLight * reflected);

        float visible = scene.occluded(position, light.position);
        float geometry = max(0.f, dot(incident, normal)) * sqDistanceInv;

        return throughput * light.radiance * visible * geometry;
    }
    else {
        return vec3(0.0f);
    }
}

vec3 pathtrace(
    Ray ray,
    const Scene& scene)
{
    vec3 throughput = vec3(1.0f);
    vec3 accum = vec3(0.0f);
    bool specular = 0;
    int bounce = 0;

    while (true) {
        auto isect = scene.intersect(ray.origin, ray.direction);

        while (scene.isLight(isect)) {
            if (bounce == 0 || specular) {
                accum += throughput * scene.lights.eval(isect);
            }

            ray.origin = isect.position;
            isect = scene.intersect(ray.origin, ray.direction);
        }

        if (!isect.isPresent()) {
            break;
        }

        auto& bsdf = scene.queryBSDF(isect);

        SurfacePoint point = scene.querySurface(isect);

        vec3 normal = point.toWorldM[1];

        mat3 lightToWorld = point.toWorldM;
        mat3 worldToLight = transpose(lightToWorld);

        accum += throughput * sampleLight(
            scene,
            isect.position,
            normal,
            -ray.direction,
            worldToLight,
            bsdf);

        auto bsdfSample = bsdf.sample(
            lightToWorld,
            worldToLight,
            -ray.direction);

        throughput *= bsdfSample.throughput * dot(normal, bsdfSample.direction);

        ray.direction = bsdfSample.direction;
        ray.origin = isect.position;

        float prob = min(0.5f, length(throughput));

        if (prob < scene.sampler.sample()) {
            break;
        }
        else {
            throughput /= prob;
        }

        ++bounce;
    }

    return accum;
}

PathTracing::PathTracing() { }

void PathTracing::setImageSize(size_t width, size_t height) {
	if (_width != width || _height != height) {
		_width = width;
		_height = height;
		softReset();
	}
}

void PathTracing::setCamera(const shared<const Camera>& camera) {
	_camera = camera;

	softReset();
}

void PathTracing::setScene(const shared<const Scene>& scene) {
	_scene = scene;

	softReset();
}

void PathTracing::softReset() {
	if (_image.size() != _width * _height) {
		_image.resize(_width * _height, vec4(0));
	}
	else {
		std::fill_n(_image.data(), _image.size(), vec4(0));
	}
}

void PathTracing::hardReset() {
	softReset();
}

void PathTracing::updateInteractive(double timeQuantum) {
	double startTime = glfwGetTime();
	size_t startRays = _scene->numRays();

	renderInteractive(_image, _width, *_camera, [&](Ray ray) -> vec3 {
        return pathtrace(ray, *_scene);
    });

	_renderTime += glfwGetTime() - startTime;
	_numRays += _scene->numRays() - startRays;
}

string PathTracing::stageName() const {
	return "Tracing paths";
}

double PathTracing::stageProgress() const {
	return _image.empty() ? 1 : atan(_image[0].w / 100.0);
}

size_t PathTracing::numRays() const {
	return _numRays;
}

double PathTracing::renderTime() const {
	return _renderTime;
}

double PathTracing::raysPerSecond() const {
	return numRays() / renderTime();
}

const vector<vec4>& PathTracing::image() const {
	return _image;
}

}
