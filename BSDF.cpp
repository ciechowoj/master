#include <BSDF.hpp>

namespace haste {

BSDFSample BSDF::sample(
    const vec3& direction) const
{
	BSDFSample sample;
    sample.direction = sampler.sample();
    sample.throughput = eval(direction, sample.direction) / one_over_two_pi<float>();
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

BSDF BSDF::lambert(const vec3& diffuse) {
	BSDF result;
	result.diffuse = diffuse / pi<float>();
	return result;
}

}
