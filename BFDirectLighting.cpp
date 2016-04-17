#include <sstream>
#include <GLFW/glfw3.h>
#include <BFDirectLighting.hpp>

namespace haste {

BFDirectLighting::BFDirectLighting() { }

void BFDirectLighting::updateInteractive(double timeQuantum) {
    double startTime = glfwGetTime();
    size_t startRays = _scene->numRays();

    _progress = renderInteractive(_image, _width, *_camera, [&](RandomEngine& source, Ray ray) -> vec3 {
        return _trace(source, ray);
    });

    _renderTime += glfwGetTime() - startTime;
    _numRays += _scene->numRays() - startRays;
}

string BFDirectLighting::stageName() const {
    std::stringstream stream;
    stream << "Computing direct lighting (" << numSamples() << "samples)";
    return stream.str();
}

double BFDirectLighting::stageProgress() const {
    return _progress;
}

vec3 BFDirectLighting::_trace(RandomEngine& source, Ray ray) {
    vec3 radiance = vec3(0.0f);

    auto isect = _scene->intersect(ray);

    while (_scene->isLight(isect)) {
        radiance += _scene->queryRadiance(isect);

        ray.origin = isect.position();
        isect = _scene->intersect(ray);
    }

    if (isect.isPresent()) {
        auto reflected = -ray.direction;
        auto surface = _scene->querySurface(isect);
        auto sample = _scene->queryBSDF(isect).sample(
            source,
            surface,
            reflected);

        ray.origin = isect.position();
        ray.direction = sample.omega();

        isect = _scene->intersect(ray);

        if (_scene->isLight(isect)) {
            return radiance += _scene->queryRadiance(isect)
                * sample.throughput()
                * dot(surface.normal(), sample.omega())
                * sample.densityInv();
        }
    }

    return radiance;
}

}
