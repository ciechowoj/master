#pragma once
#include <random>
#include <glm>

namespace haste {

struct RandomEngine {
public:
    RandomEngine();
    RandomEngine(RandomEngine&& that);

    float random1();
    vec2 random2();
    vec3 random3();
    vec4 random4();

    float sample() { return random1(); }

    std::uint_fast32_t operator()();

private:
    std::minstd_rand engine;
    RandomEngine(const RandomEngine&) = delete;
    RandomEngine& operator=(const RandomEngine&) = delete;
};

using random_generator_t = RandomEngine;

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

struct sample_lambert_bounded_result_t {
    vec3 omega;
    float adjust;
};

vec3 sample_lambert(random_generator_t& generator, vec3 omega);
sample_lambert_bounded_result_t sample_lambert_bounded(
    random_generator_t& generator,
    vec3 omega,
    bounding_sphere_t& sphere);



class lambertian_bounded_distribution_t {
public:
    lambertian_bounded_distribution_t(angular_bound_t bound);

    vec3 sample(random_generator_t& generator) const;
    float density(vec3 sample) const;
    float density_inv(vec3 sample) const;
    float subarea() const;
private:
    float _uniform_theta_inf;
    float _uniform_theta_sup;
    float _uniform_phi_inf;
    float _uniform_phi_sup;
};

struct UniformSample1 {
    float _value;

    const float a() const { return _value; }
    const float value() const { return _value; }
    const float density() const { return 1.0f; }
    const float densityInv() const { return 1.0f; }
};

struct UniformSample2 {
    vec2 _value;

    const float a() const { return _value.x; }
    const float b() const { return _value.y; }
    const vec2& value() const { return _value; }
    const float density() const { return 1.0f; }
    const float densityInv() const { return 1.0f; }
};

struct DiskSample1 {
    vec2 _point;

    const float theta() const;
    const float radius() const;
    const vec2 point() const { return _point; }
    const float density() const { return one_over_pi<float>(); }
    const float densityInv() const { return pi<float>(); }
};

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

UniformSample1 sampleUniform1(RandomEngine& engine);
UniformSample2 sampleUniform2(RandomEngine& engine);
DiskSample1 sampleDisk1(RandomEngine& engine);
HemisphereSample1 sampleHemisphere1(RandomEngine& engine);
CosineHemisphereSample1 sampleCosineHemisphere1(RandomEngine& engine);
BarycentricSample1 sampleBarycentric1(RandomEngine& engine);

}
