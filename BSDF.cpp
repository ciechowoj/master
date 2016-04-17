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
    const vec3& a,
    const vec3& b) const
{
    return query(point.toSurface(a), point.toSurface(b));
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
    const vec3& b) const
{
    return _diffuse * one_over_pi<float>();
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
    result._specular = false;

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
        result._specular = false;

        return result;
    }
    else {
        BSDFSample result;
        result._throughput = vec3(0.0f);
        result._omega = vec3(0.0f);
        result._density = 0;
        result._densityInv = 0;
        result._specular = false;

        return result;
    }
}

}
