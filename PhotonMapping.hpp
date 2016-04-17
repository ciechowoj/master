#pragma once
#include <Technique.hpp>
#include <KDTree3D.hpp>

namespace haste {

class PhotonMapping : public Technique {
public:
    PhotonMapping(size_t numPhotons, size_t numNearest, float maxDistance);

    void hardReset() override;
    void updateInteractive(
        size_t width,
        size_t height,
        vec4* image,
        double timeQuantum) override;

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
    const size_t _numNearest;
    static const size_t _maxNumNearest = 1000;
    const float _maxDistance;

    float _totalPower;
    _Stage _stage = _Scatter;
    vector<Photon> _auxiliary;
    size_t _numEmitted;
    KDTree3D<Photon> _photons;
    double _progress = 0.0;

    void _scatterPhotonsInteractive(
        size_t width,
        size_t height,
        vec4* image,
        double timeQuantum);

    void _scatterPhotons(
        RandomEngine& engine,
        size_t begin,
        size_t end);

    void _renderPhotons(
        size_t width,
        size_t height,
        vec4* image,
        size_t begin,
        size_t end);

    void _buildPhotonMapInteractive(double timeQuantum);

    void _gatherPhotonsInteractive(
        size_t width,
        size_t height,
        vec4* image,
        double timeQuantum);

    vec3 _gather(RandomEngine& source, Ray ray);

    PhotonMapping(const PhotonMapping&) = delete;
    PhotonMapping& operator=(const PhotonMapping&) = delete;
};

}
