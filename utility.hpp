#pragma once
#include <random>
#include <functional>
#include <glm>

namespace haste {

using std::function;
using namespace glm;

class UniformSampler {
public:
    UniformSampler();
    float sample();
private:
    std::minstd_rand engine;
};

class PiecewiseSampler {
public:
    PiecewiseSampler();
    PiecewiseSampler(const float* weightsBegin, const float* weightsEnd);
    float sample();
private:
    std::minstd_rand engine;
    std::piecewise_constant_distribution<float> distribution;
};

class BarycentricSampler {
public:
    vec3 sample();
private:
    UniformSampler uniform;
};

typedef function<vec3(
    const vec3& normal, 
    const vec3& tangent, 
    const vec3& in, 
    const vec3& out)> BxDF;

BxDF lambertBRDF(const vec3& diffuse);

}
