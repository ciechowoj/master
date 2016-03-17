#pragma once
#include <random>
#include <glm/glm.hpp>

namespace haste {

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

}
