#include <iostream>
#include <BSDF.hpp>
#include <Scene.hpp>

namespace haste {

BSDFSample BSDF::sample(
    const vec3& direction) const
{
    BSDFSample sample;
    sample.direction = sampler.sample();
    sample.throughput = eval(direction, sample.direction);
    sample.specular = false;
    return sample;
}

BSDFSample BSDF::sample(
        const mat3& lightToWorld,
        const mat3& worldToLight,
        const vec3& direction) const
{
    BSDFSample sample = this->sample(worldToLight * direction);
    sample.direction = lightToWorld * sample.direction;
    return sample;
}

vec3 BSDF::eval(
    const vec3& incident,
    const vec3& reflected) const
{
    return diffuse;
}

BSDFSample BSDF::sample(const SurfacePoint& point, const vec3& reflected) const {
    BSDFSample sample;
    vec3 direction = sampler.sample();
    sample.direction = point.toWorld(direction);
    sample.throughput = query(point, sample.direction, reflected) * pi<float>() * 2.0f;
    sample.specular = false;

    return sample;
}

vec3 BSDF::query(
        const SurfacePoint& point,
        const vec3& incident,
        const vec3& reflected) const
{
    return diffuse;
}

BSDF BSDF::lambert(const vec3& diffuse) {
    BSDF result;
    result.diffuse = diffuse / pi<float>();
    return result;
}

}
