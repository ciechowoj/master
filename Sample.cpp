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

      float phi_center = fract(atan2(center.z, center.x) * 0.5f * one_over_pi<float>() + 1.0f) * two_pi<float>();
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

  return {vec3(x, y, z), adjust};
}

direction_sample_t sample_lambert(random_generator_t& generator, vec3 omega,
                                  bounding_sphere_t outer,
                                  bounding_sphere_t inner) {
  auto bound = angular_bound(inner);

  // not so trivial
  // auto outer_bound = angular_bound(outer);
  // bound.theta_inf = max(outer_bound.theta_inf, bound.theta_inf);
  // bound.theta_sup = min(outer_bound.theta_sup, bound.theta_sup);
  // bound.phi_inf = max(outer_bound.phi_inf, bound.phi_inf);
  // bound.phi_sup = min(outer_bound.phi_sup, bound.phi_sup);

  auto uniform_theta_inf = cos(bound.theta_sup) * cos(bound.theta_sup);
  auto uniform_theta_sup = cos(bound.theta_inf) * cos(bound.theta_inf);
  auto uniform_phi_inf = bound.phi_inf * one_over_pi<float>() * 0.5f;
  auto uniform_phi_sup = bound.phi_sup * one_over_pi<float>() * 0.5f;

  auto theta_range = uniform_theta_sup - uniform_theta_inf;
  auto phi_range = uniform_phi_sup - uniform_phi_inf;
  float adjust = theta_range * phi_range / lambert_adjust(outer);

  float y = sqrt(generator.sample() * theta_range + uniform_theta_inf);
  float phi =
      two_pi<float>() * (generator.sample() * phi_range + uniform_phi_inf);
  float r = sqrt(1 - y * y);
  float x = r * cos(phi);
  float z = r * sin(phi);

  return {vec3(x, y, z), adjust};
}

float lambert_adjust(bounding_sphere_t sphere) {
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
  return abs(sample.direction.y) * one_over_pi<float>() / sample.adjust;
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

  float y = pow(generator.sample() * theta_range + uniform_theta_inf,
                1.0f / (power + 1.0f));
  float phi =
      two_pi<float>() * (generator.sample() * phi_range + uniform_phi_inf);
  float r = sqrt(1 - y * y);
  float x = r * cos(phi);
  float z = r * sin(phi);

  return {refl_to_surf * vec3(x, y, z), adjust};
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

direction_sample_t sample_hemisphere(random_generator_t& generator,
                                 bounding_sphere_t sphere) {
  auto bound = angular_bound(sphere);

  auto uniform_theta_inf = cos(bound.theta_sup);
  auto uniform_theta_sup = cos(bound.theta_inf);
  auto uniform_phi_inf = bound.phi_inf * one_over_pi<float>() * 0.5f;
  auto uniform_phi_sup = bound.phi_sup * one_over_pi<float>() * 0.5f;

  auto theta_range = uniform_theta_sup - uniform_theta_inf;
  auto phi_range = uniform_phi_sup - uniform_phi_inf;
  float adjust = theta_range * phi_range;

  float y = generator.sample() * theta_range + uniform_theta_inf;
  float phi =
      two_pi<float>() * (generator.sample() * phi_range + uniform_phi_inf);
  float r = sqrt(1 - y * y);
  float x = r * cos(phi);
  float z = r * sin(phi);

  return {vec3(x, y, z), adjust};
}

random_generator_t::random_generator_t() {
  std::random_device device;
  engine.seed(device());
}

random_generator_t::random_generator_t(std::size_t seed) { engine.seed(seed); }

random_generator_t::random_generator_t(random_generator_t&& that)
    : engine(std::move(that.engine)) {}

template <>
float random_generator_t::sample<float>() {
  return std::uniform_real_distribution<float>()(engine);
}

random_generator_t random_generator_t::clone() {
  return random_generator_t(this->operator()() * UINT32_MAX +
                            this->operator()());
}

template <>
vec2 random_generator_t::sample<vec2>() {
  return vec2(std::uniform_real_distribution<float>()(engine),
              std::uniform_real_distribution<float>()(engine));
}

std::uint_fast32_t random_generator_t::operator()() {
  return engine.operator()();
}
}
