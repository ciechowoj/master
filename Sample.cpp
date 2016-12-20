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

direction_sample_t sample_lambert(random_generator_t& generator, vec3 omega,
                                  bounding_sphere_t sphere) {
  auto bound = angular_bound(sphere);

  auto uniform_theta_inf = cos(bound.theta_sup) * cos(bound.theta_sup);
  auto uniform_theta_sup = cos(bound.theta_inf) * cos(bound.theta_inf);
  auto uniform_phi_inf = bound.phi_inf * one_over_pi<float>() * 0.5f;
  auto uniform_phi_sup = bound.phi_sup * one_over_pi<float>() * 0.5f;

  auto theta_range = uniform_theta_sup - uniform_theta_inf;
  auto phi_range = uniform_phi_sup - uniform_phi_inf;
  float adjust = theta_range * phi_range;

  float y = sqrt(generator.sample() * theta_range + uniform_theta_inf);
  float phi =
      two_pi<float>() * (generator.sample() * phi_range + uniform_phi_inf);
  float r = sqrt(1 - y * y);
  float x = r * cos(phi);
  float z = r * sin(phi);

  return { vec3(x, y, z), adjust };
}

float lambert_adjust(vec3 omega, bounding_sphere_t sphere) {
  auto bound = angular_bound(sphere);

  auto uniform_theta_inf = cos(bound.theta_sup) * cos(bound.theta_sup);
  auto uniform_theta_sup = cos(bound.theta_inf) * cos(bound.theta_inf);
  auto uniform_phi_inf = bound.phi_inf * one_over_pi<float>() * 0.5f;
  auto uniform_phi_sup = bound.phi_sup * one_over_pi<float>() * 0.5f;

  auto theta_range = uniform_theta_sup - uniform_theta_inf;
  auto phi_range = uniform_phi_sup - uniform_phi_inf;

  return theta_range * phi_range;
}

float lambert_density(direction_sample_t sample) {
  return sample.direction.y * one_over_pi<float>() / sample.adjust;
}

direction_sample_t sample_phong(random_generator_t& generator, vec3 omega,
                                float power) {
  mat3 refl_to_surf = reflection_to_surface(vec3(-omega.x, omega.y, -omega.z));

  float y = pow(generator.sample(), 1.0f / (power + 1.0f));
  float r = sqrt(1.0f - y * y);

  float phi = generator.sample() * 2.0f * pi<float>();
  float x = r * cos(phi);
  float z = r * sin(phi);

  return {refl_to_surf * vec3(x, y, z), 1.0f};
}

direction_sample_t sample_phong(random_generator_t& generator, vec3 omega,
                                float power, bounding_sphere_t sphere) {
  mat3 refl_to_surf = reflection_to_surface(vec3(-omega.x, omega.y, -omega.z));

  sphere.center = sphere.center * refl_to_surf;

  auto bound = angular_bound(sphere);

  auto uniform_theta_inf = pow(cos(bound.theta_sup), power + 1.0f);
  auto uniform_theta_sup = pow(cos(bound.theta_inf), power + 1.0f);
  auto uniform_phi_inf = bound.phi_inf * one_over_pi<float>() * 0.5f;
  auto uniform_phi_sup = bound.phi_sup * one_over_pi<float>() * 0.5f;

  auto theta_range = uniform_theta_sup - uniform_theta_inf;
  auto phi_range = uniform_phi_sup - uniform_phi_inf;
  float adjust = theta_range * phi_range;

  float y = pow(generator.sample() * theta_range + uniform_theta_inf, 1.0f / (power + 1.0f));
  float phi =
      two_pi<float>() * (generator.sample() * phi_range + uniform_phi_inf);
  float r = sqrt(1 - y * y);
  float x = r * cos(phi);
  float z = r * sin(phi);

  return { refl_to_surf * vec3(x, y, z), adjust };
}

float phong_adjust(vec3 omega, float power, bounding_sphere_t sphere) {
  mat3 refl_to_surf = reflection_to_surface(vec3(-omega.x, omega.y, -omega.z));

  sphere.center = sphere.center * refl_to_surf;

  auto bound = angular_bound(sphere);

  auto uniform_theta_inf = pow(cos(bound.theta_sup), power + 1.0f);
  auto uniform_theta_sup = pow(cos(bound.theta_inf), power + 1.0f);
  auto uniform_phi_inf = bound.phi_inf * one_over_pi<float>() * 0.5f;
  auto uniform_phi_sup = bound.phi_sup * one_over_pi<float>() * 0.5f;

  auto theta_range = uniform_theta_sup - uniform_theta_inf;
  auto phi_range = uniform_phi_sup - uniform_phi_inf;

  return theta_range * phi_range;
}

RandomEngine::RandomEngine() {
  std::random_device device;
  engine.seed(device());
}

RandomEngine::RandomEngine(RandomEngine&& that)
    : engine(std::move(that.engine)) {}

template <>
float RandomEngine::sample<float>() {
  return std::uniform_real_distribution<float>()(engine);
}

template <>
vec2 RandomEngine::sample<vec2>() {
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
