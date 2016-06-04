#include <sstream>
#include <GLFW/glfw3.h>
#include <DirectIllumination.hpp>

namespace haste {

DirectIllumination::DirectIllumination() { }

void DirectIllumination::render(
   ImageView& view,
   RandomEngine& engine,
   size_t cameraId)
{
    auto trace = [&](RandomEngine& engine, Ray ray) -> vec3 {
        return this->trace(engine, ray);
    };

    for_each_ray(view, engine, _scene->cameras(), cameraId, trace);
}

string DirectIllumination::name() const {
    return "Direct Illumination";
}

vec3 DirectIllumination::trace(RandomEngine& engine, Ray ray) {
    vec3 radiance = vec3(0.0f);

    auto isect = _scene->intersect(ray);

    while (isect.isLight()) {
        radiance += _scene->queryRadiance(isect);

        ray.origin = isect.position();
        isect = _scene->intersect(ray);
    }

    if (isect.isPresent()) {
        auto reflected = -ray.direction;
        auto surface = _scene->querySurface(isect);
        auto sample = _scene->queryBSDF(isect).sample(
            engine,
            surface,
            reflected);

        ray.origin = isect.position();
        ray.direction = sample.omega();

        isect = _scene->intersect(ray);

        if (isect.isLight()) {
            radiance += _scene->queryRadiance(isect)
                * sample.throughput()
                * dot(normalize(surface.normal()), sample.omega())
                / sample.density();
        }
    }

    return radiance;
}

}
