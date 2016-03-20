#pragma once
#include <scene.hpp>
#include <camera.hpp>

namespace haste {

size_t raycastInteractive(
    std::vector<vec4>& image,
    size_t pitch,
    const Camera& camera,
    const Scene& scene);

}
