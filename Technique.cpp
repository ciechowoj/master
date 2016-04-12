#include <GLFW/glfw3.h>
#include <Technique.hpp>

namespace haste {

Technique::Technique() { }

Technique::~Technique() { }

void Technique::setImageSize(size_t width, size_t height) {
    if (_width != width || _height != height) {
        _width = width;
        _height = height;
        softReset();
    }
}

void Technique::setCamera(const shared<const Camera>& camera) {
    _camera = camera;

    softReset();
}

void Technique::setScene(const shared<const Scene>& scene) {
    _scene = scene;

    softReset();
}

void Technique::softReset() {
    if (_image.size() != _width * _height) {
        _image.resize(_width * _height, vec4(0));
    }
    else {
        std::fill_n(_image.data(), _image.size(), vec4(0));
    }
}

void Technique::hardReset() {
    softReset();
}

double Technique::stageProgress() const {
    return _image.empty() ? 1 : atan(numSamples() / 100.0);
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
    return size_t(_image[0].w);
}

const vector<vec4>& Technique::image() const {
    return _image;
}

}
