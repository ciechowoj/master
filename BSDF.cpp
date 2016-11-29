#include <runtime_assert>
#include <BSDF.hpp>
#include <Scene.hpp>

namespace haste {

const bool BSDFSample::zero() const
{
    return _throughput.x + _throughput.y + _throughput.z == 0.0f;
}

const BSDFQuery BSDFSample::query() const
{
    BSDFQuery query;
    query.throughput = throughput();
    query.density = density();
    query.densityRev = densityRev();
    return query;
}

BSDF::BSDF() { }

BSDF::~BSDF() { }

const BSDFQuery BSDF::query(
        const vec3& incident,
        const vec3& outgoing) const
{
    BSDFQuery query;

    query.throughput = incident.y > 0.0f && outgoing.y > 0.0f
        ? vec3(1.0f, 0.0f, 1.0f) * one_over_pi<float>()
        : vec3(0.0f);

    query.density = outgoing.y > 0.0f
        ? outgoing.y * one_over_pi<float>()
        : 0.0f;

    query.densityRev = incident.y > 0.0f
        ? incident.y * one_over_pi<float>()
        : 0.0f;

    return query;
}

const BSDFQuery BSDF::query(
    const SurfacePoint& point,
    const vec3& incident,
    const vec3& outgoing) const
{
    return query(point.toSurface(incident), point.toSurface(outgoing));
}

const BSDFSample BSDF::sample(
    RandomEngine& engine,
    const vec3& omega) const
{
    auto hemisphere = sampleCosineHemisphere1(engine);

    BSDFSample result;
    result._throughput = vec3(1.f, 0.f, 1.f) * one_over_pi<float>();
    result._omega = hemisphere.omega();
    result._density = hemisphere.density();
    result._densityRev = abs(omega.y * one_over_pi<float>());
    result._specular = 0.0f;

    return result;
}

const BSDFBoundedSample BSDF::sampleBounded(
    RandomEngine& engine,
    const vec3& omega,
    const angular_bound_t& bound) const
{
    return BSDFBoundedSample();
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

const BSDFQuery DeltaBSDF::query(
    const vec3& incident,
    const vec3& outgoing) const
{
    BSDFQuery query;
    query.throughput = vec3(0.0f);
    query.density = 0.0f;
    query.densityRev = 0.0f;
    query.specular = 1.0f;
    return query;
}


LightBSDF::LightBSDF(vec3 radiance)
    : _radiance(radiance * one_over_pi<float>()) {
}

const BSDFSample LightBSDF::sample(
    RandomEngine& engine,
    const vec3& omega) const {
    runtime_assert(false);
    return BSDFSample();
}

const BSDFQuery LightBSDF::query(
    const vec3& incident,
    const vec3& outgoing) const {
    BSDFQuery query;

    query.throughput = outgoing.y > 0.0f ? vec3(1.0f) : vec3(0.0f);

    query.density = outgoing.y > 0.0f
        ? outgoing.y * one_over_pi<float>()
        : 0.0f;

    query.densityRev = incident.y > 0.0f
        ? incident.y * one_over_pi<float>()
        : 0.0f;

    return query;
}

const BSDFSample CameraBSDF::sample(
    RandomEngine& engine,
    const vec3& omega) const {
    runtime_assert(false);
    return BSDFSample();
}

const BSDFQuery CameraBSDF::query(
    const vec3& incident,
    const vec3& outgoing) const {
    BSDFQuery query;

    query.throughput = 1.0f / vec3(pow(incident.y * incident.y, 2));

    query.density = 0.0f;

    query.densityRev = 1.0f;

    return query;
}

}
