#include <ReflectionBSDF.hpp>

namespace haste {

const BSDFSample ReflectionBSDF::sample(
    RandomEngine& engine,
    const vec3& omega) const
{
    BSDFSample sample;
    sample._throughput = vec3(1.0f, 1.0f, 1.0f) / omega.y;
    sample._omega = vec3(0.0f, 2.0f * omega.y, 0.0f) - omega;
    sample._density = 1.0f;
    sample._densityRev = 1.0f;
    sample._specular = 1.0f;
    return sample;
}

}
