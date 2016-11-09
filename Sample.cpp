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
        }
        else if (radius_sq < distance_sq) {
            theta_inf = theta_center - theta_radius;
            theta_sup = min(half_pi<float>(), theta_center + theta_radius);

            float phi_center = atan2(center.z, center.x);
            float phi_radius = asin(radius / lateral_distance);

            phi_inf = phi_center - phi_radius;
            phi_sup = phi_center + phi_radius;
        }
    }

    return { theta_inf, theta_sup, phi_inf, phi_sup };
}

lambertian_bounded_distribution_t::lambertian_bounded_distribution_t(angular_bound_t bound) {
    _uniform_theta_inf = cos(bound.theta_sup) * cos(bound.theta_sup);
    _uniform_theta_sup = cos(bound.theta_inf) * cos(bound.theta_inf);
    _uniform_phi_inf = bound.phi_inf * one_over_pi<float>() * 0.5f;
    _uniform_phi_sup = bound.phi_sup * one_over_pi<float>() * 0.5f;
}

vec3 lambertian_bounded_distribution_t::sample(random_generator_t& generator) const {
    float theta_range = _uniform_theta_sup - _uniform_theta_inf;
    float phi_range = _uniform_phi_sup - _uniform_phi_inf;

    float y = sqrt(generator.sample() * theta_range + _uniform_theta_inf);
    float phi = two_pi<float>() * (generator.sample() * phi_range + _uniform_phi_inf);
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
    return (_uniform_theta_sup - _uniform_theta_inf) * (_uniform_phi_sup - _uniform_phi_inf);
}

RandomEngine::RandomEngine() {
    std::random_device device;
    engine.seed(device());
}

RandomEngine::RandomEngine(RandomEngine&& that)
    : engine(std::move(that.engine)) {
}

float RandomEngine::random1() {
    return std::uniform_real_distribution<float>()(engine);
}

vec2 RandomEngine::random2() {
    return vec2(
        std::uniform_real_distribution<float>()(engine),
        std::uniform_real_distribution<float>()(engine));
}

vec3 RandomEngine::random3() {
    return vec3(
        std::uniform_real_distribution<float>()(engine),
        std::uniform_real_distribution<float>()(engine),
        std::uniform_real_distribution<float>()(engine));
}

vec4 RandomEngine::random4() {
    return vec4(
        std::uniform_real_distribution<float>()(engine),
        std::uniform_real_distribution<float>()(engine),
        std::uniform_real_distribution<float>()(engine),
        std::uniform_real_distribution<float>()(engine));
}

std::uint_fast32_t RandomEngine::operator()() {
    return engine.operator()();
}

const float DiskSample1::theta() const {
    return atan2(_point.y, _point.x);
}

const float DiskSample1::radius() const {
    return length(_point);
}

UniformSample1 sampleUniform1(RandomEngine& engine) {
    return { engine.random1() };
}

UniformSample2 sampleUniform2(RandomEngine& engine) {
    return { engine.random2() };
}

DiskSample1 sampleDisk1(RandomEngine& engine) {
    auto uniform = sampleUniform2(engine);
    float sqRadius = sqrt(uniform.a());
    DiskSample1 result;
    result._point.x = cos(2.0f * pi<float>() * uniform.b()) * sqRadius;
    result._point.y = sin(2.0f * pi<float>() * uniform.b()) * sqRadius;
    return result;
}

HemisphereSample1 sampleHemisphere1(RandomEngine& engine) {
    auto uniform = sampleUniform2(engine);
    float a = uniform.a();
    float b = uniform.b() * pi<float>() * 2.0f;
    float c = sqrt(1 - a * a);
    return { vec3(cos(b) * c, a, sin(b) * c) };
}

CosineHemisphereSample1 sampleCosineHemisphere1(RandomEngine& engine) {
    auto uniform = sampleUniform2(engine);
    float r = sqrt(uniform.a());
    float phi = uniform.b() * pi<float>() * 2.0f;
    float x = r * cos(phi);
    float z = r * sin(phi);
    float y = sqrt(max(0.0f, 1.0f - x * x - z * z));
    return { vec3(x, y, z) };
}

BarycentricSample1 sampleBarycentric1(RandomEngine& engine) {
    auto uniform = sampleUniform2(engine);

    const float u = uniform.a();
    const float v = uniform.b();

    if (u + v <= 1) {
        return { vec2(u, v) };
    }
    else {
        return { vec2(1 - u, 1 - v) };
    }
}

}
