#pragma once
#include <BSDF.hpp>

namespace haste {

class PhongBSDF : public BSDF {
public:
    PhongBSDF(vec3 diffuse, vec3 specular, float power);

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
    vec3 _specular;
    float _power;
};

}
