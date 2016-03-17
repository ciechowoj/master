#include <utility.hpp>

namespace haste {

UniformSampler::UniformSampler() {
	std::random_device device;
	engine.seed(device());
}

float UniformSampler::sample() {
	return std::uniform_real_distribution<float>()(engine);
}

PiecewiseSampler::PiecewiseSampler() { }

PiecewiseSampler::PiecewiseSampler(const float* weightsBegin, const float* weightsEnd) {
	std::random_device device;
	engine.seed(device());

	size_t numWeights = weightsEnd - weightsBegin;

	distribution = std::piecewise_constant_distribution<float>(
		numWeights,
		0.f,
		1.f,
		[&](float x) { return weightsBegin[size_t(x * (numWeights + 1))]; }
		);
}

float PiecewiseSampler::sample() {
	return distribution(engine);
}

vec3 BarycentricSampler::sample() {
	float u = uniform.sample();
	float v = uniform.sample();

	if (u + v <= 1) {
		return vec3(u, v, 1 - u - v);
	}
	else {
		return vec3(1 - u, 1 - v, u + v - 1);
	}
}

}
