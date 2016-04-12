#include <PhotonMapping.hpp>

namespace haste {

PhotonMapping::PhotonMapping(size_t numPhotons)
    : _numPhotons(numPhotons) { }

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
        case _Gather: return Technique::stageProgress();
    }
}

}
