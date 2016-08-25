#pragma once
#include <BSDF.hpp>

namespace haste {

class ReflectionBSDF : public DeltaBSDF {
public:
    const BSDFSample sample(
        RandomEngine& engine,
        const vec3& omega) const override;
};

}
