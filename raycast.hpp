#pragma once
#include <Scene.hpp>
#include <Camera.hpp>

namespace haste {

size_t raycastInteractive(
    std::vector<vec4>& image,
    size_t pitch,
    const Camera& camera,
    const Scene& scene);

}
