#include <PhotonMap.hpp>
#include <scene.hpp>

namespace haste {
    
vector<LightPhoton> scatter(const Scene& scene, size_t number) {
    vector<LightPhoton> photons;

    for (size_t i = 0; i < number; ++i) {
        LightPhoton photon = scene.lights.emit();

        for (size_t i = 0; i < 3; ++i) {
            RayIsect isect = scene.intersect(
                photon.position,
                photon.direction);

            if (!isect.isPresent() || !scene.isMesh(isect)) {
                break;
            }

            photon.position = isect.position;
            photon.direction = photon.direction;
            
            //if (i == 1)
            photons.push_back(photon);

            SurfacePoint point = scene.querySurface(isect);

            if(!scene.materials.scatter(point.materialID, photon, point)) {
                break;
            }
        }
    }

    return photons;
}

}
