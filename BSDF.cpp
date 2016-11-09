#include <BSDF.hpp>
#include <Scene.hpp>

namespace haste {


angular_bound_t angular_bound(vec3 center, float radius) {
    float theta_inf = 0;
    float theta_sup = half_pi<float>();
    float phi_inf = 0;
    float phi_sup = two_pi<float>();

    float lateral_distance_sq = center.x * center.x + center.z * center.z;
    float distance_sq = lateral_distance_sq + center.y * center.y;
    float radius_sq = radius * radius;

    float lateral_distance = sqrt(lateral_distance_sq);
    float distance = sqrt(distance_sq);

    float theta_center = asin(lateral_distance / distance);
    float theta_radius = asin(radius / distance);

    if (lateral_distance_sq < radius_sq) {
        theta_sup = min(half_pi<float>(), theta_center + theta_radius);
    }
    else if (radius_sq < distance_sq) {
        theta_inf = theta_center - theta_radius;
        theta_sup = min(half_pi<float>(), theta_center + theta_radius);

        float phi_center = atan2(center.z, center.x);
        float phi_radius = asin(radius / lateral_distance);

        phi_inf = phi_center - phi_radius;
        phi_sup = phi_center + phi_radius;
    }

    return { theta_inf, theta_sup, phi_inf, phi_sup };
}

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

}
