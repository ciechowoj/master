#include <DiffuseBSDF.hpp>
#include <SurfacePoint.hpp>

namespace haste {

DiffuseBSDF::DiffuseBSDF(const vec3& diffuse) : _diffuse(diffuse) {}

const BSDFQuery DiffuseBSDF::query(vec3 incident, vec3 outgoing) const {
  float same_size = incident.y * outgoing.y > 0.0f ? 1.0f : 0.0f;

  BSDFQuery query;
  query.throughput = _diffuse * one_over_pi<float>() * same_size;
  query.density = abs(outgoing.y * one_over_pi<float>()) * same_size;
  query.densityRev = abs(incident.y * one_over_pi<float>()) * same_size;
  query.specular = 0.0f;

  return query;
}

const BSDFSample DiffuseBSDF::sample(RandomEngine& engine, vec3 omega) const {
  BSDFSample sample;
  sample.omega = sample_lambert(engine, omega);

  BSDFQuery query = this->query(omega, sample.omega);

  sample.throughput = query.throughput;
  sample.density = query.density;
  sample.densityRev = query.densityRev;
  sample.specular = query.specular;

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
