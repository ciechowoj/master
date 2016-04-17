#include <Sample.hpp>

namespace haste {

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
