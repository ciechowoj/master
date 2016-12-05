#include <ReflectionBSDF.hpp>

namespace haste {

const BSDFSample ReflectionBSDF::sample(RandomEngine& engine, vec3 omega) const
{
    BSDFSample sample;
    sample.throughput = vec3(1.0f, 1.0f, 1.0f) / omega.y;
    sample.omega =  vec3(-omega.x, omega.y, -omega.z);
    sample.density = 1.0f;
    sample.densityRev = 1.0f;
    sample.specular = 1.0f;
    return sample;
}

}
