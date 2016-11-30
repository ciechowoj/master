#include <PhongBSDF.hpp>
#include <SurfacePoint.hpp>

namespace haste {

PhongBSDF::PhongBSDF(vec3 diffuse, vec3 specular, float power)
    : _diffuse(diffuse), _specular(specular), _power(power) {}

const BSDFQuery PhongBSDF::query(vec3 incident, vec3 outgoing) const {
  BSDFQuery query;

  vec3 reflected = vec3(-incident.x, incident.y, -incident.z);
  float cos_alpha = clamp(dot(outgoing, reflected), 0.0f, 1.0f);
  float cos_alpha_pow = pow(cos_alpha, _power);

  float same_side = incident.y * outgoing.y > 0.0f ? 1.0f : 0.0f;
  const float half_over_pi = 0.5f * one_over_pi<float>();

  vec3 diffuse = _diffuse * one_over_pi<float>();
  vec3 specular = _specular * (_power + 2.0f) * half_over_pi * cos_alpha_pow;

  query.density = (_power + 1.0f) * half_over_pi * cos_alpha_pow;
  query.densityRev = query.density;
  query.throughput = same_side * (diffuse + specular);

  return query;
}

vec3 sample_phong(random_generator_t& generator, vec3 omega, float power) {
  mat3 refl_to_surf;
  refl_to_surf[1] = vec3(-omega.x, omega.y, -omega.z);
  refl_to_surf[2] =
      normalize(vec3(0.0f, 1.0f, 0.0f) - refl_to_surf[1].y * refl_to_surf[1]);
  refl_to_surf[0] = normalize(cross(refl_to_surf[1], refl_to_surf[2]));

  float y = pow(generator.sample(), 1.0f / (power + 1.0f));
  float r = sqrt(1.0f - y * y);

  float phi = generator.sample() * 2.0f * pi<float>();
  float x = r * cos(phi);
  float z = r * sin(phi);

  return refl_to_surf * vec3(x, y, z);
}

const BSDFSample PhongBSDF::sample(RandomEngine& engine, vec3 omega) const {
  BSDFSample sample;
  sample._omega = sample_phong(engine, omega, _power);
  BSDFQuery query = PhongBSDF::query(omega, sample._omega);
  sample._throughput = query.throughput;
  sample._density = query.density;
  sample._densityRev = query.densityRev;
  sample._specular = query.specular;

  return sample;
}

const BSDFBoundedSample PhongBSDF::sampleBounded(
    RandomEngine& engine, vec3 omega, const angular_bound_t& bound) const {
  auto distribution = lambertian_bounded_distribution_t(bound);

  BSDFBoundedSample result;
  result._omega = distribution.sample(engine);
  result._area = distribution.subarea();

  return result;
}
}
