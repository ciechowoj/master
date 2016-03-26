#include <runtime_assert>
#include <Lights.hpp>

namespace haste {
	
float AreaLights::queryTotalPower() const {
	vec3 power = queryTotalPower3();
	return power.x + power.y + power.z;
}

vec3 AreaLights::queryTotalPower3() const {
	runtime_assert(indices.size() % 3 == 0);

	vec3 result(0.0f);

	size_t numFaces = this->numFaces();

	for (size_t i = 0; i < numFaces; ++i) {
		result += queryAreaLightPower3(i);
	}

	return result;
}

float AreaLights::queryAreaLightPower(size_t id) const {
	vec3 power = queryAreaLightPower3(id);
	return power.x + power.y + power.z;
}

vec3 AreaLights::queryAreaLightPower3(size_t id) const {
    return exitances[id] * queryAreaLightArea(id);
}

float AreaLights::queryAreaLightArea(size_t id) const {
	vec3 u = vertices[indices[id * 3 + 1]] - vertices[indices[id * 3 + 0]];
    vec3 v = vertices[indices[id * 3 + 2]] - vertices[indices[id * 3 + 0]];
	return length(cross(u, v)) * 0.5f;
}

LightPhoton AreaLights::emit() const {
	return LightPhoton();
}

void AreaLights::buildLightStructs() const {
    size_t numFaces = this->numFaces();
    lightWeights.resize(numFaces);

    float totalPower = queryTotalPower();
    float totalPowerInv = 1.0f / totalPower;

    for (size_t i = 0; i < numFaces; ++i) {
    	float power = queryAreaLightPower(i);
        lightWeights[i] = power * totalPowerInv;
    }

    lightSampler = PiecewiseSampler(
        lightWeights.data(), 
        lightWeights.data() + lightWeights.size());

    for (size_t i = 0; i < numFaces; ++i) {
        lightWeights[i] = 1.f / lightWeights[i];
    }
}

}
