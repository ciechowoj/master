#pragma once
#include <glm>
#include <random>

namespace haste {

struct piecewise_sampler_t;

struct random_generator_t {
 public:
  random_generator_t();
  random_generator_t(std::size_t seed);
  random_generator_t(random_generator_t&& that);

  template <class T = float>
  T sample();

  random_generator_t clone();

  std::uint_fast32_t operator()();

  void seed(std::size_t seed);

 private:
  std::mt19937 engine;

  random_generator_t(const random_generator_t&) = delete;
  random_generator_t& operator=(const random_generator_t&) = delete;

  friend struct piecewise_sampler_t;
};

template <>
float random_generator_t::sample<float>();
template <>
vec2 random_generator_t::sample<vec2>();

using RandomEngine = random_generator_t;

struct piecewise_sampler_t {
 public:
  piecewise_sampler_t();
  piecewise_sampler_t(const float* weightsBegin, const float* weightsEnd);

  float sample(random_generator_t& generator);

 private:
  std::piecewise_constant_distribution<float> distribution;
};

struct bounding_sphere_t {
  vec3 center;
  float radius;
};

struct angular_bound_t {
  float theta_inf, theta_sup;
  float phi_inf, phi_sup;
};

angular_bound_t angular_bound(vec3 center, float radius);
angular_bound_t angular_bound(bounding_sphere_t sphere);

struct direction_sample_t {
  vec3 direction;
  float adjust;
};

mat3 reflection_to_surface(vec3 reflection);

direction_sample_t sample_lambert(random_generator_t& generator, vec3 omega);

direction_sample_t sample_lambert(random_generator_t& generator,
                                  bounding_sphere_t sphere, vec3 omega);

direction_sample_t sample_lambert(random_generator_t& generator, vec3 omega,
                                  bounding_sphere_t outer,
                                  bounding_sphere_t inner);

float lambert_adjust(bounding_sphere_t sphere);

float lambert_density(direction_sample_t sample);

direction_sample_t sample_phong(random_generator_t& generator, vec3 omega,
                                float power);

direction_sample_t sample_phong(random_generator_t& generator, vec3 omega,
                                float power, bounding_sphere_t sphere);

float phong_adjust(vec3 omega, float power, bounding_sphere_t sphere);

direction_sample_t sample_phong(random_generator_t& generator, vec3 omega,
                                float power, bounding_sphere_t sphere,
                                const mat3& reflection);

float phong_adjust(vec3 omega, float power, bounding_sphere_t sphere,
                   const mat3& reflection);

direction_sample_t sample_hemisphere(random_generator_t& generator,
                                     bounding_sphere_t sphere);

}
