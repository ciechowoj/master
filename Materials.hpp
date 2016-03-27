#pragma once
#include <glm>
#include <utility.hpp>
#include <BSDF.hpp>

namespace haste {
	
struct SurfacePoint;
struct LightPhoton;

class Materials {
public:
	vector<string> names;
	vector<vec3> diffuses;
	vector<vec3> speculars;
	vector<BSDF> bsdfs;

	bool scatter(
		size_t id, 
		LightPhoton& photon, 
		const SurfacePoint& point) const;

private:

};

}
