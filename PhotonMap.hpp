#pragma once
#include <KDTree3D.hpp>

namespace haste {

class Scene;

struct Photon {
    vec3 position;
    vec3 direction;
    vec3 power;

    float operator[](size_t index) const {
    	return position[index];
    }

    float& operator[](size_t index) {
    	return position[index];
    }
};

class PhotonMap {
public:
    PhotonMap();
    PhotonMap(const Scene& scene, size_t numPhotons);

    vec3 estimateRadiance(
    	const vec3& point,
    	const vec3& normal,
    	const vec3& outgoing) const;
private:
    KDTree3D<Photon> _photons;
};

}
