#include <runtime_assert>
#include <BSDF.hpp>
#include <Scene.hpp>

namespace haste {

BSDF::BSDF() { }

BSDF::~BSDF() { }

const BSDFQuery BSDF::query(vec3 incident, vec3 outgoing) const
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

const BSDFQuery BSDF::query(const SurfacePoint& point, vec3 incident, vec3 outgoing) const
{
    return query(point.toSurface(incident), point.toSurface(outgoing));
}

const BSDFSample BSDF::sample(
    RandomEngine& engine,
    vec3 omega) const
{
    BSDFSample sample;
    sample.throughput = vec3(1.0f, 0.0f, 1.0f) * one_over_pi<float>();
    sample.omega = sample_lambert(engine, omega);
    sample.density = abs(sample.omega.y * one_over_pi<float>());
    sample.densityRev = abs(omega.y * one_over_pi<float>());
    sample.specular = 0.0f;

    return sample;
}

const BSDFSample BSDF::sample(
    RandomEngine& engine,
    const SurfacePoint& point,
    vec3 omega) const
{
    BSDFSample result = sample(engine, point.toSurface(omega));
    result.omega = point.toWorld(result.omega);

    return result;
}

BSDFBoundedSample BSDF::sample_bounded(
    random_generator_t& generator,
    bounding_sphere_t target,
    vec3 omega) const {
    return BSDFBoundedSample();
}

float BSDF::gathering_density(
    random_generator_t& generator,
    const Intersector* intersector,
    const SurfacePoint& surface,
    bounding_sphere_t target,
    vec3 omega) const {
    const float L = 16777216.0f;
    float N = 1.0f;

    omega = surface.toSurface(omega);
    target.center = surface.toSurface(target.center - surface.position());

    float target_length = length(target.center);
    vec3 target_normalized = target.center / target_length;
    float cos_alpha = target_length / sqrt(target_length * target_length + target.radius * target.radius) - FLT_EPSILON;

    while (N < L) {
        auto sample = sample_bounded(generator, target, omega);

        if (cos_alpha < dot(sample.omega, target_normalized)) {
            RayIsect isect = intersector->intersectMesh(
                surface.position(),
                surface.toWorld(sample.omega),
                target_length + target.radius);

            if (isect.isPresent()) {

                vec3 tentative = surface.toSurface(isect.position() - surface.position());

                float distance_sq = distance2(target.center, tentative);

                if (distance_sq < target.radius * target.radius) {
                    return N / sample.adjust;
                }
            }
        }

        N += 1.0f;
    }

    return INFINITY;
}


const BSDFQuery DeltaBSDF::query(vec3 incident, vec3 outgoing) const
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

const BSDFSample LightBSDF::sample(RandomEngine& engine, vec3 omega) const {
    runtime_assert(false);
    return BSDFSample();
}

const BSDFQuery LightBSDF::query(vec3 incident, vec3 outgoing) const {
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

const BSDFSample CameraBSDF::sample(RandomEngine& engine, vec3 omega) const {
    runtime_assert(false);
    return BSDFSample();
}

const BSDFQuery CameraBSDF::query(vec3 incident, vec3 outgoing) const {
    BSDFQuery query;

    query.throughput = (incident.y < 0.0f ? 1.0f : 0.0f) / vec3(pow(incident.y, 4));

    query.density = 0.0f;

    query.densityRev = 1.0f;

    return query;
}

}
