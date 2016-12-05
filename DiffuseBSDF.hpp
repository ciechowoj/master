#pragma once
#include <BSDF.hpp>

namespace haste {

class DiffuseBSDF : public BSDF {
public:
    DiffuseBSDF(const vec3& diffuse);

    const BSDFQuery query(vec3 incident, vec3 outgoing) const override;

    const BSDFSample sample(
        RandomEngine& engine,
        vec3 omega) const override;

    BSDFBoundedSample sample_bounded(
        random_generator_t& generator,
        bounding_sphere_t target,
        vec3 omega) const override;

private:
    vec3 _diffuse;
};

}
