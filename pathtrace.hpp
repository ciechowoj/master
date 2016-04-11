#pragma once
#include <Scene.hpp>
#include <Camera.hpp>

namespace haste {

size_t pathtraceInteractive(
    std::vector<vec4>& image,
    size_t pitch,
    const Camera& camera,
    const Scene& scene);

}
