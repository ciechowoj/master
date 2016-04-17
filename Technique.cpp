#include <GLFW/glfw3.h>
#include <Technique.hpp>

namespace haste {

Technique::Technique() { }

Technique::~Technique() { }

void Technique::setCamera(const shared<const Camera>& camera) {
    _camera = camera;

    softReset();
}

void Technique::setScene(const shared<const Scene>& scene) {
    _scene = scene;

    hardReset();
}

void Technique::softReset() { }

void Technique::hardReset() {
    softReset();
}

double Technique::stageProgress() const {
    return atan(numSamples() / 100.0) * two_over_pi<double>();
}

size_t Technique::numRays() const {
    return _numRays;
}

double Technique::renderTime() const {
    return _renderTime;
}

double Technique::raysPerSecond() const {
    return numRays() / renderTime();
}

size_t Technique::numSamples() const {
    return _numSamples;
}

}
