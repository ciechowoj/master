#pragma once
#include <glm>
#include <vector>
#include <Lights.hpp>

namespace haste {

using std::vector;

class Scene;

vector<LightPhoton> scatter(const Scene& scene, size_t number);

class PhotonMap {
public:
    PhotonMap();
    PhotonMap(vector<LightPhoton>&& photons);
private:
    vector<LightPhoton> photons;
};

}
