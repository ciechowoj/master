#include <BSDF.hpp>
#include <Scene.hpp>
#include <runtime_assert>

namespace haste {

BSDF::BSDF() {}

BSDF::~BSDF() {}

BSDFBoundedSample BSDF::sample_bounded(random_generator_t& generator,
                                       bounding_sphere_t target,
                                       vec3 omega) const {
  runtime_assert(false);
  return BSDFBoundedSample();
}

float BSDF::gathering_density(random_generator_t& generator,
                              const Intersector* intersector,
                              const SurfacePoint& surface,
                              bounding_sphere_t target, vec3 omega) const {
  const float L = 16777216.0f;
  float N = 1.0f;

  omega = surface.toSurface(omega);
  target.center = surface.toSurface(target.center - surface.position());

  float target_length = length(target.center);

  while (N < L) {
    auto sample = sample_bounded(generator, target, omega);

    SurfacePoint isect = intersector->intersectMesh(
        surface, surface.toWorld(sample.omega), target_length + target.radius);

    if (isect.is_present()) {
      vec3 tentative = surface.toSurface(isect.position() - surface.position());

      float distance_sq = distance2(target.center, tentative);

      if (distance_sq < target.radius * target.radius) {
        return N / sample.adjust;
      }
    }

    N += 1.0f;
  }

  return INFINITY;
}

LightBSDF::LightBSDF(bounding_sphere_t sphere) : _sphere(sphere) {}

BSDFSample LightBSDF::sample(random_generator_t& generator,
                             const SurfacePoint& surface, vec3 omega) const {
  bounding_sphere_t local_sphere = {
      surface.toSurface(_sphere.center - surface.position()), _sphere.radius};

  auto sample = sample_lambert(generator, vec3(0.0f, 1.0f, 0.0f), local_sphere);

  BSDFSample result;
  result.throughput = vec3(1.0f);
  result.omega = surface.toWorld(sample.direction);
  result.density = lambert_density(sample);
  result.densityRev = 0.0f;
  result.specular = 0.0f;

  return result;
}

BSDFQuery LightBSDF::query(const SurfacePoint& surface, vec3 incident,
                           vec3 outgoing) const {
  bounding_sphere_t local_sphere = {
      surface.toSurface(_sphere.center - surface.position()), _sphere.radius};

  auto local_outgoing = surface.toSurface(outgoing);

  BSDFQuery query;

  query.throughput = local_outgoing.y > 0.0f ? vec3(1.0f) : vec3(0.0f);

  query.density = (local_outgoing.y > 0.0f ? 1.0f : 0.0f) * local_outgoing.y *
                  one_over_pi<float>() / lambert_adjust(local_sphere);

  query.densityRev = 0.0f;

  return query;
}

BSDFSample CameraBSDF::sample(random_generator_t& generator,
                              const SurfacePoint& surface, vec3 omega) const {
  runtime_assert(false);
  return BSDFSample();
}

BSDFQuery CameraBSDF::query(const SurfacePoint& surface, vec3 incident,
                            vec3 outgoing) const {
  BSDFQuery query;

  vec3 local_incident = surface.toSurface(incident);

  query.throughput =
      (local_incident.y < 0.0f ? 1.0f : 0.0f) / vec3(pow(local_incident.y, 4));

  query.density = 0.0f;

  query.densityRev = 1.0f;

  return query;
}

DiffuseBSDF::DiffuseBSDF(vec3 diffuse) : _diffuse(diffuse) {}

BSDFQuery DiffuseBSDF::query(const SurfacePoint& surface, vec3 incident,
                             vec3 outgoing) const {
  return _query(surface.toSurface(incident), surface.toSurface(outgoing));
}

BSDFSample DiffuseBSDF::sample(random_generator_t& generator,
                               const SurfacePoint& surface, vec3 omega) const {
  vec3 local_omega = surface.toSurface(omega);

  BSDFSample sample;
  sample.omega = sample_lambert(generator, local_omega).direction;

  BSDFQuery query = _query(local_omega, sample.omega);
  sample.omega = surface.toWorld(sample.omega);
  sample.throughput = query.throughput;
  sample.density = query.density;
  sample.densityRev = query.densityRev;
  sample.specular = query.specular;

  return sample;
}

BSDFBoundedSample DiffuseBSDF::sample_bounded(random_generator_t& generator,
                                              bounding_sphere_t target,
                                              vec3 omega) const {
  auto sample = sample_lambert(generator, omega, target);

  BSDFBoundedSample result;
  result.omega = sample.direction;
  result.adjust = sample.adjust;

  return result;
}

BSDFQuery DiffuseBSDF::_query(vec3 incident, vec3 outgoing) const {
  float same_side = incident.y * outgoing.y > 0.0f ? 1.0f : 0.0f;

  BSDFQuery query;
  query.throughput = _diffuse * one_over_pi<float>() * same_side;
  query.density = abs(outgoing.y * one_over_pi<float>()) * same_side;
  query.densityRev = abs(incident.y * one_over_pi<float>()) * same_side;
  query.specular = 0.0f;

  return query;
}

PhongBSDF::PhongBSDF(vec3 diffuse, vec3 specular, float power)
    : _diffuse(diffuse), _specular(specular), _power(power) {
  float diffuse_reflectivity = l1Norm(_diffuse) * one_over_pi<float>();
  float specular_reflectivity =
      l1Norm(_specular) * 2.0f * pi<float>() / (_power + 1.0f);

  float reflectivity_sum = diffuse_reflectivity + specular_reflectivity;

  _diffuse_probability = diffuse_reflectivity / reflectivity_sum;
}

BSDFQuery PhongBSDF::query(const SurfacePoint& surface, vec3 incident,
                           vec3 outgoing) const {
  return _query(surface.toSurface(incident), surface.toSurface(outgoing));
}

BSDFSample PhongBSDF::sample(random_generator_t& generator,
                             const SurfacePoint& surface, vec3 omega) const {
  vec3 local_omega = surface.toSurface(omega);

  BSDFSample sample;
  sample.omega = generator.sample() < _diffuse_probability
                     ? sample_lambert(generator, local_omega).direction
                     : sample_phong(generator, local_omega, _power).direction;

  BSDFQuery query = _query(local_omega, sample.omega);
  sample.omega = surface.toWorld(sample.omega);
  sample.throughput = query.throughput;
  sample.density = query.density;
  sample.densityRev = query.densityRev;
  sample.specular = query.specular;

  return sample;
}

BSDFQuery PhongBSDF::_query(vec3 incident, vec3 outgoing) const {
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

BSDFBoundedSample PhongBSDF::sample_bounded(random_generator_t& generator,
                                            bounding_sphere_t target,
                                            vec3 omega) const {
  BSDFBoundedSample result;

  float diffuse_adjust = lambert_adjust(target);
  float specular_adjust = phong_adjust(omega, _power, target);

  float diffuse_probability = diffuse_adjust * _diffuse_probability /
                              (diffuse_adjust * _diffuse_probability +
                               specular_adjust * (1.0f - _diffuse_probability));

  if (generator.sample() < diffuse_probability) {
    auto sample = sample_lambert(generator, omega, target);

    result.omega = sample.direction;
    result.adjust = sample.adjust * _diffuse_probability +
                    specular_adjust * (1.0f - _diffuse_probability);
  } else {
    auto sample = sample_phong(generator, omega, _power, target);

    result.omega = sample.direction;
    result.adjust = sample.adjust * (1.0f - _diffuse_probability) +
                    diffuse_adjust * _diffuse_probability;
  }

  return result;
}

BSDFQuery DeltaBSDF::query(const SurfacePoint& surface, vec3 incident,
                           vec3 outgoing) const {
  BSDFQuery query;
  query.throughput = vec3(0.0f);
  query.density = 0.0f;
  query.densityRev = 0.0f;
  query.specular = 1.0f;
  return query;
}

BSDFSample ReflectionBSDF::sample(random_generator_t& generator,
                                  const SurfacePoint& surface,
                                  vec3 omega) const {
  BSDFSample sample;
  sample.throughput = vec3(1.0f, 1.0f, 1.0f) / omega.y;
  sample.omega = vec3(-omega.x, omega.y, -omega.z);
  sample.density = 1.0f;
  sample.densityRev = 1.0f;
  sample.specular = 1.0f;
  return sample;
}

TransmissionBSDF::TransmissionBSDF(float internalIOR, float externalIOR) {
  externalOverInternalIOR = externalIOR / internalIOR;
  this->internalIOR = internalIOR;
}

BSDFSample TransmissionBSDF::sample(random_generator_t& generator,
                                    const SurfacePoint& surface,
                                    vec3 reflected) const {
  vec3 omega;

  if (reflected.y > 0.f) {
    const float eta = externalOverInternalIOR;

    omega =
        -eta * (reflected - vec3(0.0f, reflected.y, 0.0f)) -
        vec3(0.0f, sqrt(1 - eta * eta * (1 - reflected.y * reflected.y)), 0.0f);
  } else {
    const float eta = 1.0f / externalOverInternalIOR;

    omega =
        -eta * (reflected - vec3(0.0f, reflected.y, 0.0f)) +
        vec3(0.0f, sqrt(1 - eta * eta * (1 - reflected.y * reflected.y)), 0.0f);
  }

  BSDFSample result;
  result.throughput = vec3(1.0f, 1.0f, 1.0f) / abs(omega.y);
  result.omega = omega;
  result.density = 1.0f;
  result.densityRev = 1.0f;
  result.specular = 1.0f;

  return result;
}
}
