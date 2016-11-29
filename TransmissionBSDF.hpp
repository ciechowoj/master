#pragma once
#include <BSDF.hpp>

namespace haste {

class TransmissionBSDF : public DeltaBSDF {
public:
    TransmissionBSDF(
        float internalIOR,
        float externalIOR);

    const BSDFSample sample(RandomEngine& engine, vec3 omega) const override;

private:
    float externalOverInternalIOR;
    float internalIOR;
};

}
