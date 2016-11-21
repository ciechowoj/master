#include <PT.hpp>

namespace haste {

PathTracing::PathTracing(size_t minSubpath, float roulette)
    : _minLength(minSubpath), _roulette(roulette) { }

void PathTracing::render(
    ImageView& view,
    render_context_t& context,
    size_t cameraId)
{
    auto trace = [&](render_context_t& context, Ray ray) -> vec3 {
        return this->trace(*context.engine, ray);
    };

    for_each_ray(view, context, _scene->cameras(), cameraId, trace);
}

vec3 PathTracing::trace(RandomEngine& engine, Ray ray) {
    return trace(engine, ray, *_scene);
}

vec3 PathTracing::trace(RandomEngine& engine, Ray ray, const Scene& scene) {
    vec3 throughput = vec3(1.0f);
    vec3 radiance = vec3(0.0f);
    float specular = 1.0f;
    int bounce = 0;

    while (true) {
        RayIsect isect = scene.intersect(ray.origin, ray.direction);

        while (isect.isLight()) {
            if (specular == 1.0f) {
                radiance += throughput * scene.queryRadiance(isect, -ray.direction);
            }

            ray.origin = isect.position();
            isect = scene.intersect(ray.origin, ray.direction);
        }

        if (!isect.isPresent()) {
            break;
        }

        const BSDF& bsdf = scene.queryBSDF(isect);
        SurfacePoint surface = scene.querySurface(isect);

        vec3 lightSample = scene.sampleDirectLightMixed(engine, surface, -ray.direction, bsdf);

        radiance += lightSample * throughput;

        BSDFSample bsdfSample = bsdf.sample(engine, surface, -ray.direction);

        specular = bsdfSample.specular();

        throughput *= bsdfSample.throughput() *
            abs(dot(surface.normal(), bsdfSample.omega())) /
            bsdfSample.density();

        ray.direction = bsdfSample.omega();
        ray.origin = isect.position();

        float threshold = bounce > _minLength ? _roulette : 1.0f;

        if (threshold < scene.sampler.sample()) {
            break;
        }

        throughput /= threshold;
        ++bounce;
    }

    return radiance;
}

string PathTracing::name() const {
    return "Path Tracing";
}

}
