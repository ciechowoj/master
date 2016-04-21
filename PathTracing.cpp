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

        DirectLightSample lightSample = _scene->sampleDirectLightArea(
            engine,
            point,
            -ray.direction,
            bsdf);

        radiance += lightSample.radiance() * lightSample.densityInv();

        auto bsdfSample = bsdf.sample(
            engine,
            point,
            -ray.direction);

        throughput *= bsdfSample.throughput() * dot(point.normal(), bsdfSample.omega()) * bsdfSample.densityInv();

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

/*PathTracing::PathTracing() { }

void PathTracing::updateInteractive(ImageView& view) {
    double startTime = glfwGetTime();
    size_t startRays = _scene->numRays();

    _progress = renderInteractive(view, _scene->cameras(), _activeCameraId, [&](RandomEngine& source, Ray ray) -> vec3 {
        return pathtrace(source, ray, *_scene);
    });

    _numSamples = size_t(view.last().w);
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
*/
}
