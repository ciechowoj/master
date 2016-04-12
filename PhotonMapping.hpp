#pragma once
#include <Technique.hpp>

namespace haste {

class PhotonMapping : public Technique {
public:
    PhotonMapping(size_t numPhotons);

    void hardReset() override;
    void updateInteractive(double timeQuantum) override;

    string stageName() const override;
    double stageProgress() const override;
private:
    enum _Stage {
        _Scatter,
        _Build,
        _BuildDone,
        _Gather
    };

    const size_t _numPhotons;
    _Stage _stage = _Scatter;
    vector<Photon> _scattered;
    PhotonMap _photons;

    PhotonMapping(const PhotonMapping&) = delete;
    PhotonMapping& operator=(const PhotonMapping&) = delete;
};

}
