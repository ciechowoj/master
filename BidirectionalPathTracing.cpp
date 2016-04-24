#include <sstream>
#include <GLFW/glfw3.h>
#include <BidirectionalPathTracing.hpp>

namespace haste {

BidirectionalPathTracing::BidirectionalPathTracing() { }

void BidirectionalPathTracing::render(
   ImageView& view,
   RandomEngine& engine,
   size_t cameraId)
{
    auto trace = [&](RandomEngine& engine, Ray ray) -> vec3 {
        return this->trace(engine, ray);
    };

    for_each_ray(view, engine, _scene->cameras(), cameraId, trace);
}

vec3 BidirectionalPathTracing::trace(RandomEngine& engine, Ray ray) {
    Vertex lightSubpath[_maxSubpathLength];
    Vertex eyeSubpath[_maxSubpathLength];

    size_t lightSubpathSize = 0;
    size_t eyeSubpathSize = 0;

    lightSubpathSize = traceLightSubpath(lightSubpath);
    eyeSubpathSize = traceEyeSubpath(eyeSubpath, ray);






    return vec3(1.0f, 0.0f, 1.0f);
}

size_t BidirectionalPathTracing::traceLightSubpath(Vertex* subpath) {




    return 0;
}

size_t BidirectionalPathTracing::traceEyeSubpath(Vertex* subpath, Ray ray) {
    return 0;
}

string BidirectionalPathTracing::name() const {
    return "Bidirectional Path Tracing";
}

}
