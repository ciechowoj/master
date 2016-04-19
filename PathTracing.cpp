#include <sstream>
#include <GLFW/glfw3.h>
#include <PathTracing.hpp>

namespace haste {

vec3 pathtrace(
    RandomEngine& engine,
    Ray ray,
    const Scene& scene)
{
    const Scene* _scene = &scene;

    vec3 throughput = vec3(1.0f);
    vec3 radiance = vec3(0.0f);
    bool specular = 0;
    int bounce = 0;

    while (true) {
        auto isect = scene.intersect(ray.origin, ray.direction);

        while (isect.isLight()) {
            if (bounce == 0 || specular) {
                radiance += throughput * _scene->queryRadiance(isect);
            }

            ray.origin = isect.position();
            isect = scene.intersect(ray.origin, ray.direction);
        }

        if (!isect.isPresent()) {
            break;
        }

        auto& bsdf = scene.queryBSDF(isect);
        SurfacePoint point = scene.querySurface(isect);

        vec3 normal = point.toWorldM[1];

        auto lightSample = _scene->sampleLight(engine, point.position);
        auto localThroughput = bsdf.query(point, lightSample.omega(), -ray.direction);

        auto cosineTheta = dot(normal, lightSample.omega());

        if (cosineTheta > 0.0f) {
            radiance +=
                throughput *
                localThroughput *
                lightSample.radiance() *
                cosineTheta *
                lightSample.densityInv();
        }

        auto bsdfSample = bsdf.sample(
            engine,
            point,
            -ray.direction);

        throughput *= bsdfSample.throughput() * dot(normal, bsdfSample.omega()) * bsdfSample.densityInv();

        ray.direction = bsdfSample.omega();
        ray.origin = isect.position();

        float prob = min(0.5f, length(throughput));

        if (prob < scene.sampler.sample()) {
            break;
        }
        else {
            throughput /= prob;
        }

        ++bounce;
    }

    return radiance;
}

PathTracing::PathTracing() { }

void PathTracing::updateInteractive(
    size_t width,
    size_t height,
    vec4* image,
    double timeQuantum) {
    double startTime = glfwGetTime();
    size_t startRays = _scene->numRays();

    _progress = renderInteractive(width, height, image, *_camera, [&](RandomEngine& source, Ray ray) -> vec3 {
        return pathtrace(source, ray, *_scene);
    });

    _numSamples = size_t(image[width * height - 1].w);
    _renderTime += glfwGetTime() - startTime;
    _numRays += _scene->numRays() - startRays;
}

string PathTracing::stageName() const {
    std::stringstream stream;
    stream << "Tracing paths (" << numSamples() << " samples)";
    return stream.str();
}

double PathTracing::stageProgress() const {
    return _progress;
}

}
