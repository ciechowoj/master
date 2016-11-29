#pragma once
#include <BSDF.hpp>

namespace haste {

class DiffuseBSDF : public BSDF {
public:
    DiffuseBSDF(const vec3& diffuse);

    const BSDFQuery query(
        const vec3& incident,
        const vec3& outgoing) const override;

    const BSDFSample sample(
        RandomEngine& engine,
        const vec3& omega) const override;

    const BSDFBoundedSample sampleBounded(
        RandomEngine& engine,
        const vec3& omega,
        const angular_bound_t& bound) const override;

private:
    vec3 _diffuse;
};

}
