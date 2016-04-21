#include <sstream>
#include <GLFW/glfw3.h>
#include <BidirectionalPathTracing.hpp>

namespace haste {

/*BidirectionalPathTracing::BidirectionalPathTracing() { }

void BidirectionalPathTracing::updateInteractive(ImageView& view)
{
    double startTime = glfwGetTime();
    size_t startRays = _scene->numRays();

    _progress = renderInteractive(view, _scene->cameras(), _activeCameraId, [&](RandomEngine& engine, Ray ray) -> vec3 {
        return _pathtrace(engine, ray);
    });

    _numSamples = size_t(view.last().w);
    _renderTime += glfwGetTime() - startTime;
    _numRays += _scene->numRays() - startRays;
}

string BidirectionalPathTracing::stageName() const {
    std::stringstream stream;
    stream << "Bidirectional path tracing (" << numSamples() << " samples)";
    return stream.str();
}

double BidirectionalPathTracing::stageProgress() const {
    return _progress;
}

vec3 BidirectionalPathTracing::_pathtrace(RandomEngine& engine, Ray ray) {
    return vec3(0.0f, 1.0f, 0.0f);
}*/

}
