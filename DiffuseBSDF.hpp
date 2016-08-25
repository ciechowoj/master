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

    BSDFSample scatter(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omega) const override;

private:
    vec3 _diffuse;
};

}
