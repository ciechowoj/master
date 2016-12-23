#pragma once
#include <glm>
#include <random>

namespace haste {

struct random_generator_t {
 public:
  random_generator_t();
  random_generator_t(std::size_t seed);
  random_generator_t(random_generator_t&& that);

  template <class T = float>
  T sample();

  random_generator_t clone();

  std::uint_fast32_t operator()();

 private:
  std::mt19937 engine;
  random_generator_t(const random_generator_t&) = delete;
  random_generator_t& operator=(const random_generator_t&) = delete;
};

template <>
float random_generator_t::sample<float>();
template <>
vec2 random_generator_t::sample<vec2>();

using RandomEngine = random_generator_t;

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

direction_sample_t sample_lambert(random_generator_t& generator, vec3 omega,
                                  bounding_sphere_t sphere);

float lambert_adjust(bounding_sphere_t sphere);

float lambert_density(direction_sample_t sample);

direction_sample_t sample_phong(random_generator_t& generator, vec3 omega,
                                float power);

direction_sample_t sample_phong(random_generator_t& generator, vec3 omega,
                                float power, bounding_sphere_t sphere);

float phong_adjust(vec3 omega, float power, bounding_sphere_t sphere);

struct HemisphereSample1 {
  vec3 _omega;

  const vec3 omega() const { return _omega; }
  const float density() const { return one_over_pi<float>() * 0.5f; }
  const float densityInv() const { return 2.0f * pi<float>(); }
};

struct CosineHemisphereSample1 {
  vec3 _omega;

  const vec3 omega() const { return _omega; }
  const float density() const { return _omega.y * one_over_pi<float>(); }
  const float densityInv() const { return pi<float>() / _omega.y; }
};

struct BarycentricSample1 {
  vec2 _uv;

  const float u() const { return _uv.x; }
  const float v() const { return _uv.y; }
  const float w() const { return 1.0f - _uv.x - _uv.y; }
  const vec3 value() const { return vec3(u(), v(), w()); }
  const float density() const { return 1.0f; }
  const float densityInv() const { return 1.0f; }
};

HemisphereSample1 sampleHemisphere1(RandomEngine& engine);
CosineHemisphereSample1 sampleCosineHemisphere1(RandomEngine& engine);
BarycentricSample1 sampleBarycentric1(RandomEngine& engine);
}
