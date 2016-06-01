#include <BSDF.hpp>
#include <Scene.hpp>

namespace haste {

const bool BSDFSample::zero() const
{
    return _throughput.x + _throughput.y + _throughput.z == 0.0f;
}

BSDF::BSDF() { }
BSDF::~BSDF() { }

const vec3 BSDF::query(
    const SurfacePoint& point,
    const vec3& incident,
    const vec3& reflected,
    const vec3& normal) const
{
    return query(
        point.toSurface(incident),
        point.toSurface(reflected),
        point.toSurface(normal));
}

const float BSDF::density(
        const vec3& incident,
        const vec3& reflected,
        const vec3& normal) const
{
    return 0.0f;
}

const float BSDF::density(
    const SurfacePoint& point,
    const vec3& incident,
    const vec3& reflected) const
{
    return density(
        point.toSurface(incident),
        point.toSurface(reflected),
        point.toSurface(point.normal()));
}

const BSDFSample BSDF::sample(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& omega) const
{
    BSDFSample result = sample(engine, point.toSurface(omega));
    result._omega = point.toWorld(result.omega());
    return result;
}

DiffuseBSDF::DiffuseBSDF(const vec3& diffuse)
    : _diffuse(diffuse) {
}

const vec3 DiffuseBSDF::query(
    const vec3& a,
    const vec3& b,
    const vec3& n) const
{
    return dot(a, n) > 0.0f && dot(b, n) > 0.0f
        ? _diffuse * one_over_pi<float>()
        : vec3(0.0f);
}

const float DiffuseBSDF::density(
    const vec3& incident,
    const vec3& reflected,
    const vec3& normal) const
{
    return incident.y > 0.0f ? incident.y * one_over_pi<float>() : 0.0f;
}

const BSDFSample DiffuseBSDF::sample(
    RandomEngine& engine,
    const vec3& omega) const
{
    auto hemisphere = sampleCosineHemisphere1(engine);

    BSDFSample result;
    result._throughput = _diffuse * one_over_pi<float>();
    result._omega = hemisphere.omega();
    result._density = hemisphere.density();
    result._densityInv = hemisphere.densityInv();
    result._specular = 0.0f;

    return result;
}

BSDFSample DiffuseBSDF::scatter(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& omega) const
{
    const float inv3 = 1.0f / 3.0f;

    float diffuseAvg = (_diffuse.x + _diffuse.y + _diffuse.z) * inv3;

    if (sampleUniform1(engine).value() < diffuseAvg) {
        auto hemisphere = sampleCosineHemisphere1(engine);

        BSDFSample result;
        result._throughput = _diffuse / diffuseAvg;
        result._omega = point.toWorld(hemisphere.omega());
        result._density = hemisphere.density();
        result._densityInv = hemisphere.densityInv();
        result._specular = 0.0f;

        return result;
    }
    else {
        BSDFSample result;
        result._throughput = vec3(0.0f);
        result._omega = vec3(0.0f);
        result._density = 0;
        result._densityInv = 0;
        result._specular = 0.0f;

        return result;
    }
}

const vec3 PerfectReflectionBSDF::query(
    const vec3& incident,
    const vec3& reflected,
    const vec3& normal) const
{
    return vec3(0.0f);
}

const float PerfectReflectionBSDF::density(
    const vec3& incident,
    const vec3& reflected,
    const vec3& normal) const
{
    return 0.0f;
}

const BSDFSample PerfectReflectionBSDF::sample(
    RandomEngine& engine,
    const vec3& reflected) const
{
    BSDFSample result;
    result._throughput = vec3(1.0f, 1.0f, 1.0f);
    result._omega = vec3(0.0f, 2.0f * reflected.y, 0.0f) - reflected;
    result._density = 1.0f;
    result._densityInv = 1.0f;
    result._specular = 1.0f;

    return result;
}

BSDFSample PerfectReflectionBSDF::scatter(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& incident) const
{
    return BSDFSample();
}

PerfectTransmissionBSDF::PerfectTransmissionBSDF(
    float internalIOR,
    float externalIOR)
{
    externalOverInternalIOR = externalIOR / internalIOR;
    this->internalIOR = internalIOR;
}

const vec3 PerfectTransmissionBSDF::query(
    const vec3& incident,
    const vec3& reflected,
    const vec3& normal) const
{
    return vec3(0.0f);
}

const float PerfectTransmissionBSDF::density(
    const vec3& incident,
    const vec3& reflected,
    const vec3& normal) const
{
    return 0.0f;
}

const BSDFSample PerfectTransmissionBSDF::sample(
    RandomEngine& engine,
    const vec3& reflected) const
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
    result._throughput = vec3(1.0f, 1.0f, 1.0f);
    result._omega = omega;
    result._density = 1.0f;
    result._densityInv = 1.0f;
    result._specular = 1.0f;

    return result;
}

BSDFSample PerfectTransmissionBSDF::scatter(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& incident) const
{
    return BSDFSample();
}

}
