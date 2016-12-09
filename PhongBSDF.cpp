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

  _diffuse_probability = diffuse_reflectivity / reflectivity_sum;
}

const BSDFQuery PhongBSDF::query(vec3 incident, vec3 outgoing) const {
  float diffuse_density_factor = _diffuse_probability;
  float specular_density_factor = 1.0f - _diffuse_probability;

  float same_side = incident.y * outgoing.y > 0.0f ? 1.0f : 0.0f;

  // diffuse
  float diffuse_density = abs(outgoing.y * one_over_pi<float>());
  float diffuse_density_rev = abs(incident.y * one_over_pi<float>());

  vec3 diffuse = _diffuse * one_over_pi<float>();

  // specular
  const float half_over_pi = 0.5f * one_over_pi<float>();
  vec3 reflected = vec3(-incident.x, incident.y, -incident.z);
  float cos_alpha = clamp(dot(outgoing, reflected), 0.0f, 1.0f);
  float cos_alpha_pow = pow(cos_alpha, _power);

  float specular_density = (_power + 1.0f) * half_over_pi * cos_alpha_pow;
  float specular_density_rev = specular_density;

  vec3 specular = _specular * (_power + 2.0f) * half_over_pi * cos_alpha_pow;

  BSDFQuery query;

  query.density = same_side * (specular_density * specular_density_factor +
                               diffuse_density * diffuse_density_factor);

  query.densityRev =
      same_side * (specular_density_rev * specular_density_factor +
                   diffuse_density_rev * diffuse_density_factor);

  query.throughput = same_side * (diffuse + specular);

  query.specular = 0.0f;

  return query;
}

const BSDFSample PhongBSDF::sample(RandomEngine& engine, vec3 omega) const {
  BSDFSample sample;
  sample.omega = engine.sample() < _diffuse_probability
                     ? sample_lambert(engine, omega).direction
                     : sample_phong(engine, omega, _power).direction;

  BSDFQuery query = PhongBSDF::query(omega, sample.omega);
  sample.throughput = query.throughput;
  sample.density = query.density;
  sample.densityRev = query.densityRev;
  sample.specular = query.specular;

  return sample;
}

BSDFBoundedSample PhongBSDF::sample_bounded(random_generator_t& generator,
                                            bounding_sphere_t target,
                                            vec3 omega) const {
  BSDFBoundedSample result;

  float diffuse_adjust = lambert_adjust(omega, target);
  float specular_adjust = phong_adjust(omega, _power, target);

  float diffuse_probability = diffuse_adjust * _diffuse_probability /
                              (diffuse_adjust * _diffuse_probability +
                               specular_adjust * (1.0f - _diffuse_probability));

  float specular_probability = 1.0f - diffuse_probability;

  if (generator.sample() < diffuse_probability) {
    auto sample = sample_lambert(generator, omega, target);

    result.omega = sample.direction;
    result.adjust = sample.adjust * _diffuse_probability +
                    specular_adjust * (1.0f - _diffuse_probability);
  } else {
    auto sample = sample_phong(generator, omega, _power, target);

    result.omega = sample.direction;
    result.adjust =
        sample.adjust * (1.0f - _diffuse_probability) +
        diffuse_adjust * _diffuse_probability;
  }

  return result;
}
}
