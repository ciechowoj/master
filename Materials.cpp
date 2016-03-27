#include <Materials.hpp>
#include <scene.hpp>

namespace haste {
    
bool Materials::scatter(
    size_t id, 
    LightPhoton& photon, 
    const SurfacePoint& point) const {
    photon.direction = reflect(photon.direction, point.toWorldM[1]);
}

}
