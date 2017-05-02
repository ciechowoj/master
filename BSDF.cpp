#include <BSDF.hpp>
#include <Scene.hpp>
#include <runtime_assert>
#include <Sample.inl>

namespace haste {

inline bool intersect(const vec3& ray, const bounding_sphere_t& sphere,
                      float length) {
  float t = dot(normalize(ray), sphere.center);
  float r = sphere.radius;
  float d_sq = length * length - t * t;

  return length <= r || (t >= 0.0f && r * r >= d_sq);
}

BSDFQuery BSDFQuery::reverse() const {
  BSDFQuery result;
  result.throughput = throughput;
  result.density = densityRev;
  result.densityRev = density;
  result.finite = finite;
  result.glossiness = glossiness;
  return result;
}

BSDF::BSDF() {}

BSDF::~BSDF() {}

BSDFBoundedSample BSDF::sample_bounded(random_generator_t& generator,
                                       const SurfacePoint& surface,
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

  bounding_sphere_t surface_target;
  surface_target.center = surface.toSurface(target.center - surface.position());
  surface_target.radius = target.radius;
  omega = surface.toSurface(omega);

  while (N < L) {
    auto sample = sample_bounded(generator, surface, surface_target, omega);

    if (intersector->intersectFast(surface, surface.toWorld(sample.omega),
                                   surface_target, target)) {
      return N / sample.adjust;
    }

    N += 1.0f;
  }

  return INFINITY;
}

uint32_t BSDF::light_id() const {
  runtime_assert(false);
  return UINT32_MAX;
}

LightBSDF::LightBSDF(bounding_sphere_t sphere, uint32_t light_id)
    : _sphere(sphere), _light_id(light_id) {}

BSDFSample LightBSDF::sample(random_generator_t& generator,
                             const SurfacePoint& surface, vec3 omega) const {
  bounding_sphere_t local_sphere = {
      surface.toSurface(_sphere.center - surface.position()), _sphere.radius};

  auto sample =
      sample_lambert(generator, local_sphere, surface.toSurface(omega));

  BSDFSample result;
  result.throughput = vec3(1.0f);
  result.omega = surface.toWorld(sample.direction);
  result.density = lambert_density(sample);
  result.densityRev = 0.0f;
  result.finite = 1;
  result.glossiness = 0.0f;

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

  query.glossiness = 0.0f;

  return query;
}

float LightBSDF::gathering_density(random_generator_t& generator,
                                   const Intersector* intersector,
                                   const SurfacePoint& surface,
                                   bounding_sphere_t target, vec3 omega) const {
  const float L = 16777216.0f;
  float N = 1.0f;

  bounding_sphere_t surface_target;
  surface_target.center = surface.toSurface(target.center - surface.position());
  surface_target.radius = target.radius;
  omega = surface.toSurface(omega);

  bounding_sphere_t local_sphere = {
      surface.toSurface(_sphere.center - surface.position()), _sphere.radius};

  while (N < L) {
    auto sample =
        sample_lambert(generator, omega, local_sphere, surface_target);

    if (intersector->intersectFast(surface, surface.toWorld(sample.direction),
                                   surface_target, target)) {
      return N / sample.adjust;
    }

    N += 1.0f;
  }

  return INFINITY;
}

/*BSDFBoundedSample LightBSDF::sample_bounded(random_generator_t& generator,
                                            const SurfacePoint& surface,
                                            bounding_sphere_t target,
                                            vec3 omega) const {
  bounding_sphere_t local_sphere = {
      surface.toSurface(_sphere.center - surface.position()), _sphere.radius};

  auto sample = sample_lambert(generator, omega, local_sphere, target);

  BSDFBoundedSample result;
  result.omega = sample.direction;
  result.adjust = sample.adjust;

  return result;
}*/

uint32_t LightBSDF::light_id() const { return _light_id; }

sun_light_bsdf::sun_light_bsdf(bounding_sphere_t sphere, uint32_t light_id)
    : _sphere(sphere), _light_id(light_id) {}

BSDFSample sun_light_bsdf::sample(random_generator_t& generator,
                                  const SurfacePoint& surface,
                                  vec3 omega) const {
  BSDFSample result;
  result.throughput = vec3(1.0f);
  result.omega = omega;
  result.density = 1.0f;
  result.densityRev = 0.0f;
  result.finite = 1;
  result.glossiness = USHRT_MAX;

  return result;
}

BSDFQuery sun_light_bsdf::query(const SurfacePoint& surface, vec3 incident,
                                vec3 outgoing) const {
  BSDFQuery query;
  query.throughput = vec3(0.0f);
  query.density = 1.0f;
  query.densityRev = 1.0f;
  query.finite = 1;
  query.glossiness = USHRT_MAX;

  return query;
}

uint32_t sun_light_bsdf::light_id() const { return _light_id; }

CameraBSDF::CameraBSDF() {}

BSDFSample CameraBSDF::sample(random_generator_t& generator,
                              const SurfacePoint& surface, vec3 omega) const {
  BSDFSample sample;
  sample.omega = -omega;
  sample.throughput = vec3(1.0f) / abs(dot(surface.normal(), omega));
  sample.density = 1.0f;
  sample.densityRev = 0.0f;
  sample.finite = 1;
  sample.glossiness = USHRT_MAX - 1;

  return sample;
}

BSDFQuery CameraBSDF::query(const SurfacePoint& surface, vec3 incident,
                            vec3 outgoing) const {
  vec3 local_incident = surface.toSurface(incident);

  BSDFQuery query;
  query.throughput = vec3(local_incident.y > 0.0f ? 1.0f : 0.0f) /
                     pow(abs(local_incident.y), 1.0f);
  query.density = 0.0f;
  query.densityRev = 1.0f;
  query.glossiness = USHRT_MAX - 1;

  return query;
}

BSDFBoundedSample CameraBSDF::sample_bounded(random_generator_t& generator,
                                             const SurfacePoint& surface,
                                             bounding_sphere_t target,
                                             vec3 omega) const {
  auto sample = sample_hemisphere(generator, target);

  BSDFBoundedSample result;
  result.omega = sample.direction;
  result.adjust = sample.adjust * 2.0f * pi<float>();

  return result;
}

DiffuseBSDF::DiffuseBSDF(vec3 diffuse) : _diffuse(diffuse) {}

BSDFQuery DiffuseBSDF::query(const SurfacePoint& surface, vec3 incident,
                             vec3 outgoing) const {
  return _query(surface.toSurface(surface.gnormal), surface.toSurface(incident),
                surface.toSurface(outgoing));
}

BSDFSample DiffuseBSDF::sample(random_generator_t& generator,
                               const SurfacePoint& surface, vec3 omega) const {
  vec3 local_omega = surface.toSurface(omega);

  BSDFSample sample;
  sample.omega = sample_lambert(generator, local_omega).direction;

  BSDFQuery query =
      _query(surface.toSurface(surface.gnormal), local_omega, sample.omega);
  sample.omega = surface.toWorld(sample.omega);
  sample.throughput = query.throughput;
  sample.density = query.density;
  sample.densityRev = query.densityRev;
  sample.finite = query.finite;
  sample.glossiness = query.glossiness;

  return sample;
}

float DiffuseBSDF::gathering_density(random_generator_t& generator,
                                     const Intersector* intersector,
                                     const SurfacePoint& surface,
                                     bounding_sphere_t target,
                                     vec3 omega) const {
  const float L = 16777216.0f;
  float N = 1.0f;

  bounding_sphere_t surface_target;
  surface_target.center = surface.toSurface(target.center - surface.position());
  surface_target.radius = target.radius;
  omega = surface.toSurface(omega);

  while (N < L) {
    auto sample = sample_lambert(generator, surface_target, omega);

    if (intersector->intersectFast(surface, surface.toWorld(sample.direction),
                                   surface_target, target)) {
      return N / sample.adjust;
    }

    N += 1.0f;
  }

  return INFINITY;
}

BSDFQuery DiffuseBSDF::_query(vec3 gnormal, vec3 incident,
                              vec3 outgoing) const {
  float same_side =
      dot(incident, gnormal) * dot(outgoing, gnormal) > 0.0f ? 1.0f : 0.0f;

  BSDFQuery query;
  query.throughput = _diffuse * one_over_pi<float>() * same_side;
  query.density = abs(outgoing.y * one_over_pi<float>()) * same_side;
  query.densityRev = abs(incident.y * one_over_pi<float>()) * same_side;
  query.finite = 1;
  query.glossiness = 0.0f;

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
  float same_side =
      dot(incident, surface.gnormal) * dot(outgoing, surface.gnormal) > 0.0f
          ? 1.0f
          : 0.0f;

  return _query(surface.toSurface(incident), surface.toSurface(outgoing),
                same_side);
}

BSDFSample PhongBSDF::sample(random_generator_t& generator,
                             const SurfacePoint& surface, vec3 omega) const {
  vec3 local_omega = surface.toSurface(omega);

  vec3 direction = generator.sample() < _diffuse_probability
                       ? sample_lambert(generator, local_omega).direction
                       : sample_phong(generator, local_omega, _power).direction;

  BSDFSample sample;
  sample.omega = surface.toWorld(direction);

  float same_side =
      dot(omega, surface.gnormal) * dot(sample.omega, surface.gnormal) > 0.0f
          ? 1.0f
          : 0.0f;

  BSDFQuery query = _query(local_omega, direction, same_side);
  sample.throughput = query.throughput;
  sample.density = query.density;
  sample.densityRev = query.densityRev;
  sample.finite = query.finite;
  sample.glossiness = query.glossiness;

  return sample;
}

BSDFQuery PhongBSDF::_query(vec3 incident, vec3 outgoing,
                            float same_side) const {
  float diffuse_density_factor = _diffuse_probability;
  float specular_density_factor = 1.0f - _diffuse_probability;

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

  query.finite = 1;
  query.glossiness = _power;

  return query;
}

float PhongBSDF::gathering_density(random_generator_t& generator,
                                   const Intersector* intersector,
                                   const SurfacePoint& surface,
                                   bounding_sphere_t target, vec3 omega) const {
  const float L = 16777216.0f;
  float N = 1.0f;

  bounding_sphere_t surface_target;
  surface_target.center = surface.toSurface(target.center - surface.position());
  surface_target.radius = target.radius;
  omega = surface.toSurface(omega);

  float diffuse_adjust = lambert_adjust(surface_target);
  float specular_adjust = phong_adjust(omega, _power, surface_target);
  float combined_adjust = diffuse_adjust * _diffuse_probability +
                          specular_adjust * (1.0f - _diffuse_probability);

  float diffuse_probability = diffuse_adjust * _diffuse_probability /
                              (diffuse_adjust * _diffuse_probability +
                               specular_adjust * (1.0f - _diffuse_probability));

  while (N < L) {
    BSDFBoundedSample result;

    if (generator.sample() < diffuse_probability) {
      auto sample = sample_lambert(generator, surface_target, omega);
      result.omega = sample.direction;
    } else {
      auto sample = sample_phong(generator, omega, _power, surface_target);
      result.omega = sample.direction;
    }

    if (intersector->intersectFast(surface, surface.toWorld(result.omega),
                                   surface_target, target)) {
      return N / combined_adjust;
    }

    N += 1.0f;
  }

  return 0.0f; // INFINITY
}

DeltaBSDF::DeltaBSDF() {}

BSDFQuery DeltaBSDF::query(const SurfacePoint& surface, vec3 incident,
                           vec3 outgoing) const {
  BSDFQuery query;
  query.throughput = vec3(0.0f);
  query.density = 0.0f;
  query.densityRev = 0.0f;
  query.finite = 0;
  query.glossiness = USHRT_MAX;

  return query;
}

BSDFSample ReflectionBSDF::sample(random_generator_t& generator,
                                  const SurfacePoint& surface,
                                  vec3 omega) const {
  vec3 local_omega = surface.toSurface(omega);

  BSDFSample sample;
  sample.throughput = vec3(1.0f, 1.0f, 1.0f) / local_omega.y;
  sample.omega =
      surface.toWorld(vec3(-local_omega.x, local_omega.y, -local_omega.z));
  sample.density = 1.0f;
  sample.densityRev = 1.0f;
  sample.finite = 0;
  sample.glossiness = USHRT_MAX;

  return sample;
}

TransmissionBSDF::TransmissionBSDF(float internalIOR, float externalIOR) {
  externalOverInternalIOR = externalIOR / internalIOR;
  this->internalIOR = internalIOR;
}

BSDFSample TransmissionBSDF::sample(random_generator_t& generator,
                                    const SurfacePoint& surface,
                                    vec3 reflected) const {
  vec3 local_omega = surface.toSurface(reflected);

  vec3 omega;

  if (local_omega.y > 0.f) {
    const float eta = externalOverInternalIOR;

    omega =
        -eta * (local_omega - vec3(0.0f, local_omega.y, 0.0f)) -
        vec3(0.0f, sqrt(1 - eta * eta * (1 - local_omega.y * local_omega.y)),
             0.0f);
  } else {
    const float eta = 1.0f / externalOverInternalIOR;

    omega =
        -eta * (local_omega - vec3(0.0f, local_omega.y, 0.0f)) +
        vec3(0.0f, sqrt(1 - eta * eta * (1 - local_omega.y * local_omega.y)),
             0.0f);
  }

  BSDFSample result;
  result.throughput = vec3(1.0f, 1.0f, 1.0f) / abs(omega.y);
  result.omega = surface.toWorld(omega);
  result.density = 1.0f;
  result.densityRev = 1.0f;
  result.finite = 0;
  result.glossiness = USHRT_MAX;

  return result;
}
}
