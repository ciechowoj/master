#pragma once
#include <BSDF.hpp>

namespace haste {

class TransmissionBSDF : public DeltaBSDF {
public:
    TransmissionBSDF(
        float internalIOR,
        float externalIOR);

    const BSDFSample sample(
        RandomEngine& engine,
        const vec3& omega) const override;

    const BSDFSample sampleAdjoint(
        RandomEngine& engine,
        const vec3& omega) const override;

private:
    float externalOverInternalIOR;
    float internalIOR;
};

}
