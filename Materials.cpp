#include <runtime_assert>
#include <Materials.hpp>
#include <Scene.hpp>

namespace haste {

bool Materials::scatter(
    Photon& photon,
    const SurfacePoint& point) const
{
    const float inv3 = 1.0f / 3.0f;
    size_t id = point.materialID;
    runtime_assert(id < diffuses.size());

    vec3 diffuse = diffuses[id];
    float diffuseAvg = (diffuse.x + diffuse.y + diffuse.z) * inv3;

    if (uniformSampler.sample() < diffuseAvg) {
        float diffuseAvgInv = 1.0f / diffuseAvg;
        photon.power = photon.power * diffuse * diffuseAvgInv;
        photon.direction = point.toWorldM * cosineSampler.sample();

        return true;
    }
    else {
        return false;
    }
}

}
