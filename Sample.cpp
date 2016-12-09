#include <Sample.hpp>

namespace haste {

angular_bound_t angular_bound(vec3 center, float radius) {
  float theta_inf = 0;
  float theta_sup = half_pi<float>();
  float phi_inf = 0;
  float phi_sup = two_pi<float>();

  float lateral_distance_sq = center.x * center.x + center.z * center.z;
  float distance_sq = lateral_distance_sq + center.y * center.y;
  float radius_sq = radius * radius;

  if (radius_sq < distance_sq) {
    float lateral_distance = sqrt(lateral_distance_sq);
    float distance = sqrt(distance_sq);

    float theta_center = asin(lateral_distance / distance);
    float theta_radius = asin(radius / distance);

    if (lateral_distance_sq < radius_sq) {
      theta_sup = min(half_pi<float>(), theta_center + theta_radius);
    } else if (radius_sq < distance_sq) {
      theta_inf = theta_center - theta_radius;
      theta_sup = min(half_pi<float>(), theta_center + theta_radius);

      float phi_center = atan2(center.z, center.x);
      float phi_radius = asin(radius / lateral_distance);

      phi_inf = phi_center - phi_radius;
      phi_sup = phi_center + phi_radius;
    }
  }

  return {theta_inf, theta_sup, phi_inf, phi_sup};
}

angular_bound_t angular_bound(bounding_sphere_t sphere) {
  return angular_bound(sphere.center, sphere.radius);
}

mat3 reflection_to_surface(vec3 reflection) {
  mat3 matrix;
  matrix[1] = reflection;
  matrix[2] = normalize(vec3(0.0f, 1.0f, 0.0f) - matrix[1].y * matrix[1]);
  matrix[0] = normalize(cross(matrix[1], matrix[2]));

  return matrix;
}

direction_sample_t sample_lambert(random_generator_t& generator, vec3 omega) {
  float y = sqrt(generator.sample()) * (omega.y > 0.0f ? 1.0f : -1.0f);
  float r = sqrt(1.0f - y * y);
  float phi = generator.sample() * 2.0f * pi<float>();
  float x = r * cos(phi);
  float z = r * sin(phi);

  return {vec3(x, y, z), 1.0f};
}

direction_sample_t sample_phong(random_generator_t& generator, vec3 omega,
                                float power) {
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

  return {refl_to_surf * vec3(x, y, z), 1.0f};
}

lambertian_bounded_distribution_t::lambertian_bounded_distribution_t(
    angular_bound_t bound) {
  _uniform_theta_inf = cos(bound.theta_sup) * cos(bound.theta_sup);
  _uniform_theta_sup = cos(bound.theta_inf) * cos(bound.theta_inf);
  _uniform_phi_inf = bound.phi_inf * one_over_pi<float>() * 0.5f;
  _uniform_phi_sup = bound.phi_sup * one_over_pi<float>() * 0.5f;
}

vec3 lambertian_bounded_distribution_t::sample(
    random_generator_t& generator) const {
  float theta_range = _uniform_theta_sup - _uniform_theta_inf;
  float phi_range = _uniform_phi_sup - _uniform_phi_inf;

  float y = sqrt(generator.sample() * theta_range + _uniform_theta_inf);
  float phi =
      two_pi<float>() * (generator.sample() * phi_range + _uniform_phi_inf);
  float r = sqrt(1 - y * y);
  float x = r * cos(phi);
  float z = r * sin(phi);
  return vec3(x, y, z);
}

float lambertian_bounded_distribution_t::density(vec3 sample) const {
  return sample.y * one_over_pi<float>();
}

float lambertian_bounded_distribution_t::density_inv(vec3 sample) const {
  return pi<float>() / sample.y;
}

float lambertian_bounded_distribution_t::subarea() const {
  return (_uniform_theta_sup - _uniform_theta_inf) *
         (_uniform_phi_sup - _uniform_phi_inf);
}

RandomEngine::RandomEngine() {
  std::random_device device;
  engine.seed(device());
}

RandomEngine::RandomEngine(RandomEngine&& that)
    : engine(std::move(that.engine)) {}

template <> float RandomEngine::sample<float>() {
  return std::uniform_real_distribution<float>()(engine);
}

template <> vec2 RandomEngine::sample<vec2>() {
    return vec2(std::uniform_real_distribution<float>()(engine),
        std::uniform_real_distribution<float>()(engine));
}

std::uint_fast32_t RandomEngine::operator()() { return engine.operator()(); }

HemisphereSample1 sampleHemisphere1(RandomEngine& engine) {
  auto uniform = engine.sample<vec2>();
  float a = uniform.x;
  float b = uniform.y * pi<float>() * 2.0f;
  float c = sqrt(1 - a * a);
  return {vec3(cos(b) * c, a, sin(b) * c)};
}

CosineHemisphereSample1 sampleCosineHemisphere1(RandomEngine& engine) {
  auto uniform = engine.sample<vec2>();
  float r = sqrt(uniform.x);
  float phi = uniform.y * pi<float>() * 2.0f;
  float x = r * cos(phi);
  float z = r * sin(phi);
  float y = sqrt(max(0.0f, 1.0f - x * x - z * z));
  return {vec3(x, y, z)};
}

BarycentricSample1 sampleBarycentric1(RandomEngine& engine) {
  auto uniform = engine.sample<vec2>();

  const float u = uniform.x;
  const float v = uniform.y;

  if (u + v <= 1) {
    return {vec2(u, v)};
  } else {
    return {vec2(1 - u, 1 - v)};
  }
}
}
