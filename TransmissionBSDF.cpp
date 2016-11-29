#include <TransmissionBSDF.hpp>

namespace haste {

TransmissionBSDF::TransmissionBSDF(
    float internalIOR,
    float externalIOR)
{
    externalOverInternalIOR = externalIOR / internalIOR;
    this->internalIOR = internalIOR;
}

const BSDFSample TransmissionBSDF::sample(RandomEngine& engine, vec3 reflected) const
{
    vec3 omega;

    if (reflected.y > 0.f) {
        const float eta = externalOverInternalIOR;

        omega =
            - eta * (reflected - vec3(0.0f, reflected.y, 0.0f))
            - vec3(0.0f, sqrt(1 - eta * eta * (1 - reflected.y * reflected.y)), 0.0f);
    }
    else {
        const float eta = 1.0f / externalOverInternalIOR;

        omega =
            - eta * (reflected - vec3(0.0f, reflected.y, 0.0f))
            + vec3(0.0f, sqrt(1 - eta * eta * (1 - reflected.y * reflected.y)), 0.0f);
    }

    BSDFSample result;
    result._throughput = vec3(1.0f, 1.0f, 1.0f) / abs(omega.y);
    result._omega = omega;
    result._density = 1.0f;
    result._densityRev = 1.0f;
    result._specular = 1.0f;

    return result;
}

}
