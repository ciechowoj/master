#include <PhotonMap.hpp>
#include <Scene.hpp>

namespace haste {

PhotonMap::PhotonMap() {
}

PhotonMap::PhotonMap(const Scene& scene, size_t numPhotons) {
    vector<Photon> photons;

    float totalPower = scene.lights.queryTotalPower();
    float numPhotonsInv = 1.0f / float(numPhotons);
    float scaleFactor = totalPower * numPhotonsInv;

    for (size_t i = 0; i < numPhotons; ++i) {
        Photon photon = scene.lights.emit();
        photon.power *= scaleFactor;

        for (size_t i = 0; ; ++i) {
            RayIsect isect = scene.intersect(
                photon.position,
                photon.direction);

            if (!isect.isPresent() || !scene.isMesh(isect)) {
                break;
            }

            photon.position = isect.position;
            photon.direction = -photon.direction;

            photons.push_back(photon);

            SurfacePoint point = scene.querySurface(isect);

            if(!scene.materials.scatter(photon, point)) {
                break;
            }
        }
    }

    _photons = KDTree3D<Photon>(photons);
}

}
