#pragma once
#include <glm>
#include <utility.hpp>

namespace haste {
	
struct SurfacePoint;
struct LightPhoton;

class Materials {
public:
	vector<string> names;
	vector<vec3> diffuses;

	bool scatter(size_t id, LightPhoton& photon, const SurfacePoint& point) const;

private:

};

}
