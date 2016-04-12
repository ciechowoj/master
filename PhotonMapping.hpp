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
    const float _numPhotonsInv;
    float _totalPower;
    _Stage _stage = _Scatter;
    vector<Photon> _auxiliary;
    size_t _numEmitted;
    PhotonMap _photons;

    void _scatterPhotonsInteractive(double timeQuantum);
    void _scatterPhotons(size_t begin, size_t end);
    void _renderPhotons(size_t begin, size_t end);
    void _buildPhotonMapInteractive(double timeQuantum);
    void _gatherPhotonsInteractive(double timeQuantum);

    PhotonMapping(const PhotonMapping&) = delete;
    PhotonMapping& operator=(const PhotonMapping&) = delete;
};

}
