#include <PhongBSDF.hpp>
#include <SurfacePoint.hpp>
#include <iostream>

namespace haste {

PhongBSDF::PhongBSDF(vec3 diffuse, vec3 specular, float power)
    : _diffuse(diffuse), _specular(specular), _power(power) {
  float diffuse_reflectivity = l1Norm(_diffuse) * one_over_pi<float>();
  float specular_reflectivity =
      l1Norm(_specular) * 2.0f * pi<float>() / (_power + 1.0f);

  float reflectivity_sum = diffuse_reflectivity + specular_reflectivity;

  _kdiffuse = diffuse_reflectivity / reflectivity_sum;
}

const BSDFQuery PhongBSDF::query(vec3 incident, vec3 outgoing) const {
  float diffuse_density_factor = _kdiffuse;
  float specular_density_factor = 1.0f - _kdiffuse;

  float same_side = incident.y * outgoing.y > 0.0f ? 1.0f : 0.0f;

  // diffuse
  float diffuse_density = outgoing.y * one_over_pi<float>();
  float diffuse_density_rev = incident.y * one_over_pi<float>();

  vec3 diffuse = _diffuse * one_over_pi<float>();

  // specular
  const float half_over_pi = 0.5f * one_over_pi<float>();
  vec3 reflected = vec3(-incident.x, incident.y, -incident.z);
  float cos_alpha = clamp(dot(outgoing, reflected), 0.0f, 1.0f);
  float cos_alpha_pow = pow(cos_alpha, _power);

  float specular_density = (_power + 1.0f) * half_over_pi * cos_alpha_pow;

  vec3 specular = _specular * (_power + 2.0f) * half_over_pi * cos_alpha_pow;

  BSDFQuery query;

  query.density = specular_density * specular_density_factor +
                  diffuse_density * diffuse_density_factor;

  query.densityRev = specular_density * specular_density_factor +
                     diffuse_density_rev * diffuse_density_factor;

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
  sample._omega = engine.sample() < _kdiffuse
                      ? sample_lambert(engine, omega)
                      : sample_phong(engine, omega, _power);

  BSDFQuery query = PhongBSDF::query(omega, sample._omega);
  sample._throughput = query.throughput;
  sample._density = query.density;
  sample._densityRev = query.densityRev;
  sample._specular = query.specular;

  return sample;
}

BSDFBoundedSample PhongBSDF::sample_bounded(random_generator_t& generator,
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
