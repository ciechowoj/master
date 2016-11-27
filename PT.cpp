#include <PT.hpp>

namespace haste {

PathTracing::PathTracing(size_t minSubpath, float roulette, size_t num_threads)
    : Technique(num_threads), _minLength(minSubpath), _roulette(roulette) { }

vec3 PathTracing::_traceEye(render_context_t& context, Ray ray) {
    vec3 throughput = vec3(1.0f);
    vec3 radiance = vec3(0.0f);
    float specular = 1.0f;
    unsigned bounce = 0;

    while (true) {
        RayIsect isect = _scene->intersect(ray.origin, ray.direction);

        while (isect.isLight()) {
            if (specular == 1.0f) {
                radiance += throughput * _scene->queryRadiance(isect, -ray.direction);
            }

            ray.origin = isect.position();
            isect = _scene->intersect(ray.origin, ray.direction);
        }

        if (!isect.isPresent()) {
            break;
        }

        const BSDF& bsdf = _scene->queryBSDF(isect);
        SurfacePoint surface = _scene->querySurface(isect);

        vec3 lightSample = _scene->sampleDirectLightMixed(*context.engine, surface, -ray.direction, bsdf);

        radiance += lightSample * throughput;

        BSDFSample bsdfSample = bsdf.sample(*context.engine, surface, -ray.direction);

        specular = bsdfSample.specular();

        throughput *= bsdfSample.throughput() *
            abs(dot(surface.normal(), bsdfSample.omega())) /
            bsdfSample.density();

        ray.direction = bsdfSample.omega();
        ray.origin = isect.position();

        float threshold = bounce > _minLength ? _roulette : 1.0f;

        if (threshold < _scene->sampler.sample()) {
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
