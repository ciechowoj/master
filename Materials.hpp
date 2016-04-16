#pragma once
#include <glm>
#include <utility.hpp>
#include <BSDF.hpp>

namespace haste {

struct SurfacePoint;
struct Photon;

class Materials {
public:
    vector<string> names;
    vector<vec3> diffuses;
    vector<vec3> emissives;
    vector<vec3> speculars;
    vector<BSDF> bsdfs;

    bool scatter(
        Photon& photon,
        const SurfacePoint& point) const;



    size_t numMaterials() const {
    	return names.size();
    }

    const string& name(size_t index) const {
    	return names[index];
    }

private:
	mutable UniformSampler uniformSampler;
	mutable HemisphereCosineSampler cosineSampler;
};

}
