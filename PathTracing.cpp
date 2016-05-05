#include <iostream>
#include <sstream>
#include <GLFW/glfw3.h>
#include <PathTracing.hpp>

namespace haste {

PathTracing::PathTracing() { }

void PathTracing::render(
    ImageView& view,
    RandomEngine& engine,
    size_t cameraId)
{
    auto trace = [&](RandomEngine& engine, Ray ray) -> vec3 {
        return this->trace(engine, ray);
    };

    for_each_ray(view, engine, _scene->cameras(), cameraId, trace);
}

vec3 PathTracing::trace(RandomEngine& engine, Ray ray) {
    vec3 throughput = vec3(1.0f);
    vec3 radiance = vec3(0.0f);
    bool specular = 0;
    int bounce = 0;

    while (true) {
        auto isect = _scene->intersect(ray.origin, ray.direction);

        while (isect.isLight()) {
            if (bounce == 0 || specular) {
                radiance += throughput * _scene->queryRadiance(isect);
            }

            ray.origin = isect.position();
            isect = _scene->intersect(ray.origin, ray.direction);
        }

        if (!isect.isPresent()) {
            break;
        }

        auto& bsdf = _scene->queryBSDF(isect);
        SurfacePoint point = _scene->querySurface(isect);

        vec3 lightSample = _scene->sampleDirectLightAngle(
            engine,
            point,
            -ray.direction,
            bsdf);

        // radiance = point.toSurface(point.toWorld(point.normal()));
        radiance += lightSample * throughput;

        auto bsdfSample = bsdf.sample(
            engine,
            point,
            -ray.direction);

        throughput *= bsdfSample.throughput() *
            dot(point.normal(), bsdfSample.omega()) *
            bsdfSample.densityInv();

        ray.direction = bsdfSample.omega();
        ray.origin = isect.position();

        float prob = bounce > 8 ? 0.5f : 1.0f;

        if (prob < _scene->sampler.sample()) {
            break;
        }
        else {
            throughput /= prob;
        }

        ++bounce;
    }

    return radiance;
}

string PathTracing::name() const {
    return "Path Tracing";
}

}
