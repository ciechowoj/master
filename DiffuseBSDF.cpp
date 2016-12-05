#include <DiffuseBSDF.hpp>
#include <SurfacePoint.hpp>

namespace haste {

DiffuseBSDF::DiffuseBSDF(const vec3& diffuse) : _diffuse(diffuse) {}

const BSDFQuery DiffuseBSDF::query(vec3 incident, vec3 outgoing) const {
  BSDFQuery query;

  query.throughput = incident.y > 0.0f && outgoing.y > 0.0f
                         ? _diffuse * one_over_pi<float>()
                         : vec3(0.0f);

  query.density = outgoing.y > 0.0f ? outgoing.y * one_over_pi<float>() : 0.0f;

  query.densityRev =
      incident.y > 0.0f ? incident.y * one_over_pi<float>() : 0.0f;

  return query;
}

const BSDFSample DiffuseBSDF::sample(RandomEngine& engine, vec3 omega) const {
  BSDFSample sample;
  sample._throughput = _diffuse * one_over_pi<float>();
  sample._omega = sample_lambert(engine, omega);
  sample._density = abs(sample._omega.y * one_over_pi<float>());
  sample._densityRev = abs(omega.y * one_over_pi<float>());
  sample._specular = 0.0f;

  return sample;
}

BSDFBoundedSample DiffuseBSDF::sample_bounded(random_generator_t& generator,
                                       bounding_sphere_t target,
                                       vec3 omega) const {
  auto bound = angular_bound(target);
  auto distribution = lambertian_bounded_distribution_t(bound);

  BSDFBoundedSample result;
  result.omega = distribution.sample(generator);
  result.adjust = distribution.subarea();

  return result;
}

}
