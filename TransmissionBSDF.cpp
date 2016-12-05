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
    result.throughput = vec3(1.0f, 1.0f, 1.0f) / abs(omega.y);
    result.omega = omega;
    result.density = 1.0f;
    result.densityRev = 1.0f;
    result.specular = 1.0f;

    return result;
}

}
