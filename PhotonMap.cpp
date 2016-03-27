#include <PhotonMap.hpp>
#include <scene.hpp>

namespace haste {
    
vector<LightPhoton> scatter(const Scene& scene, size_t number) {
    vector<LightPhoton> photons;

    for (size_t i = 0; i < number; ++i) {
        LightPhoton photon = scene.lights.emit();

        while (true) {
            RayIsect isect = scene.intersect(
                photon.position,
                photon.direction);

            if (!isect.isPresent()) {
                break;
            }

            photon.position = isect.position;
            photon.direction = -photon.direction;
            photons.push_back(photon);

            // SurfacePoint point = scene.querySurface(isect);

            /*scene.materials.scatter(photon, isect.primID, point);

            auto& bsdf = scene.material().bsdf;

            if(!bsdf.scatter(photon)) {
                break;
            }*/
        }



    }



    return photons;
}

}
