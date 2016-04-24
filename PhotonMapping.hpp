#pragma once
#include <Technique.hpp>
#include <KDTree3D.hpp>

namespace haste {

class PhotonMapping : public Technique {
public:
    PhotonMapping(size_t numPhotons, size_t numNearest, float maxDistance);

    void preprocess(
        const shared<const Scene>& scene,
        RandomEngine& engine,
        const function<void(string, float)>& progress,
        bool parallel) override;

    void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId) override;

    string name() const override;

private:
    const size_t _numPhotons;
    const float _numPhotonsInv;
    const size_t _numNearest;
    static const size_t _maxNumNearest = 1000;
    const float _maxDistance;

    float _totalPower;
    vector<Photon> _auxiliary;
    size_t _numEmitted;
    KDTree3D<Photon> _photons;

    void _renderPhotons(
        ImageView& view,
        size_t cameraId,
        size_t begin,
        size_t end);

    void _scatterPhotons(
        RandomEngine& engine,
        size_t begin,
        size_t end);

    void _buildPhotonMap();

    vec3 _gather(RandomEngine& source, Ray ray);

    PhotonMapping(const PhotonMapping&) = delete;
    PhotonMapping& operator=(const PhotonMapping&) = delete;
};

}
