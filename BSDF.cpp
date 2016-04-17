#include <BSDF.hpp>
#include <Scene.hpp>

namespace haste {

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
    RandomEngine& source,
    const SurfacePoint& point,
    const vec3& omega) const
{
    BSDFSample result = sample(source, point.toSurface(omega));
    result._omega = point.toWorld(result.omega());
    return result;
}

DiffuseBSDF::DiffuseBSDF(const vec3& diffuse)
    : _diffuse(diffuse * one_over_pi<float>()) {
}

const vec3 DiffuseBSDF::query(
    const vec3& a,
    const vec3& b) const {
    return _diffuse;
}

const BSDFSample DiffuseBSDF::sample(
    RandomEngine& source,
    const vec3& omega) const {
    auto hemisphere = sampleCosineHemisphere1(source);

    BSDFSample result;
    result._throughput = _diffuse;
    result._omega = hemisphere.omega();
    result._density = hemisphere.density();
    result._densityInv = hemisphere.densityInv();
    result._specular = false;

    return result;
}

BSDFSample DiffuseBSDF::scatter(
    RandomEngine& source,
    const SurfacePoint& point,
    const vec3& omega) const {
    return BSDFSample();
}

}
