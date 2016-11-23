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
    query._throughput = throughput();
    query._density = density();
    query._densityRev = densityRev();
    return query;
}

BSDF::BSDF() { }

BSDF::~BSDF() { }

const BSDFQuery BSDF::query(
        const vec3& incident,
        const vec3& outgoing) const
{
    BSDFQuery query;

    query._throughput = incident.y > 0.0f && outgoing.y > 0.0f
        ? vec3(1.0f, 0.0f, 1.0f) * one_over_pi<float>()
        : vec3(0.0f);

    query._density = outgoing.y > 0.0f
        ? outgoing.y * one_over_pi<float>()
        : 0.0f;

    query._densityRev = incident.y > 0.0f
        ? incident.y * one_over_pi<float>()
        : 0.0f;

    return query;
}

const BSDFQuery BSDF::queryAdjoint(
    const vec3& incident,
    const vec3& outgoing) const
{
    return query(incident, outgoing);
}

const BSDFQuery BSDF::query(
    const SurfacePoint& point,
    const vec3& incident,
    const vec3& outgoing) const
{
    return query(point.toSurface(incident), point.toSurface(outgoing));
}

const BSDFQuery BSDF::queryAdjoint(
    const SurfacePoint& point,
    const vec3& incident,
    const vec3& outgoing) const
{
    return queryAdjoint(point.toSurface(incident), point.toSurface(outgoing));
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

const BSDFSample BSDF::sampleAdjoint(
    RandomEngine& engine,
    const vec3& omega) const
{
    return sample(engine, omega);
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

const BSDFSample BSDF::sampleAdjoint(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& omega) const
{
    BSDFSample result = sampleAdjoint(engine, point.toSurface(omega));
    result._omega = point.toWorld(result.omega());
    return result;
}

BSDFSample BSDF::scatter(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& omega) const
{
    return sample(engine, point, omega);
}

const BSDFQuery DeltaBSDF::query(
    const vec3& incident,
    const vec3& outgoing) const
{
	BSDFQuery query;
    query._throughput = vec3(0.0f);
    query._density = 0.0f;
    query._densityRev = 0.0f;
    return query;
}

const BSDFQuery DeltaBSDF::queryAdjoint(
    const vec3& incident,
    const vec3& outgoing) const
{
	BSDFQuery query;
    query._throughput = vec3(0.0f);
    query._density = 0.0f;
    query._densityRev = 0.0f;
    return query;
}

const BSDFSample LightBSDF::sample(
    RandomEngine& engine,
    const vec3& omega) const {
    return BSDFSample();
}

const BSDFQuery LightBSDF::query(
    const vec3& incident,
    const vec3& outgoing) const {
    BSDFQuery query;

    query._throughput = outgoing.y > 0.0f ? _radiance : vec3(0.0f);

    query._density = outgoing.y > 0.0f
        ? outgoing.y * one_over_pi<float>()
        : 0.0f;

    query._densityRev = 0.0f;

    return query;
}

}
