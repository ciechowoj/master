#include <PhotonMapping.hpp>

namespace haste {

PhotonMapping::PhotonMapping(size_t numPhotons)
    : _numPhotons(numPhotons) { }

void PhotonMapping::setImageSize(size_t width, size_t height) {
    _width = width;
    _height = height;

    softReset();
}

void PhotonMapping::setCamera(const shared<const Camera>& camera) {
    _camera = camera;

    softReset();
}

void PhotonMapping::setScene(const shared<const Scene>& scene) {
    _scene = scene;

    hardReset();
}

void PhotonMapping::softReset() {
    if (_image.size() != _width * _height) {
        _image.resize(_width * _height, vec4(0, 0, 0, 0));
    }
    else {
        std::fill_n(_image.data(), _image.size(), vec4(0, 0, 0, 0));
    }
}

void PhotonMapping::hardReset() {
    softReset();
    _stage = _Scatter;
}

void PhotonMapping::updateInteractive(double timeQuantum) {




}

string PhotonMapping::stageName() const {
    switch(_stage) {
        case _Scatter: return "Scattering photons";
        case _Build:
        case _BuildDone: return "Building photon map";
        case _Gather: return "Gathering photons";
    }
}

double PhotonMapping::stageProgress() const {
    switch(_stage) {
        case _Scatter: return double(_scattered.size()) / _numPhotons;
        case _Build: return 0.0;
        case _BuildDone: return 1.0;
        case _Gather: _image.empty() ? 1 : atan(_image[0].w / 100.0);
    }
}

size_t PhotonMapping::numRays() const {
    return 0;
}

double PhotonMapping::renderTime() const {
    return 0.0;
}

double PhotonMapping::raysPerSecond() const {
    return numRays() / renderTime();
}

const vector<vec4>& PhotonMapping::image() const {
    return _image;
}

}
