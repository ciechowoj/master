#pragma once
#include <glm>
#include <vector>

namespace haste {

using std::vector;

struct Photon {
	vec3 position;
	vec3 color;
	vec3 incident;
	unsigned flags;
};

class Scene;

vector<Photon> scatter(const Scene& scene, size_t number);

class PhotonMap {
public:
	PhotonMap();
	PhotonMap(vector<Photon>&& photons);
private:
	vector<Photon> photons;
};

}
