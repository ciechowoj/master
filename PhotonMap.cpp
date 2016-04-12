#include <PhotonMap.hpp>
#include <Scene.hpp>

namespace haste {

PhotonMap::PhotonMap() {
}

PhotonMap::PhotonMap(vector<Photon>&& photons) {
    _photons = KDTree3D<Photon>(std::move(photons));
}

}
