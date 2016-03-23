#include <Lights.hpp>

namespace haste {
	
vec3 AreaLights::queryTotalPower() const {
	vec3 result(0.0f);

	for (size_t i = 0; i < vertices.size(); i += 3) {

	}

	return result;
}

LightPhoton AreaLights::emit() const {
	return LightPhoton();
}

}
