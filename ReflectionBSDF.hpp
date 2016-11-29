#pragma once
#include <BSDF.hpp>

namespace haste {

class ReflectionBSDF : public DeltaBSDF {
public:
    const BSDFSample sample(RandomEngine& engine, vec3 omega) const override;
};

}
