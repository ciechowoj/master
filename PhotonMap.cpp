#include <PhotonMap.hpp>
#include <scene.hpp>

namespace haste {

vector<LightPhoton> scatter(const Scene& scene, size_t number) {
    vector<LightPhoton> photons;

    float totalPower = scene.lights.queryTotalPower();
    float numberInv = 1.0f / float(number);
    float scaleFactor = totalPower * numberInv;

    for (size_t i = 0; i < number; ++i) {
        LightPhoton photon = scene.lights.emit();
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

    return photons;
}

}
