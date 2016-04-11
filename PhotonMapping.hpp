#pragma once
#include <Camera.hpp>
#include <Scene.hpp>

namespace haste {

class PhotonMapping {
public:
    PhotonMapping(size_t numPhotons);

    void setImageSize(size_t width, size_t height);
    void setCamera(const shared<const Camera>& camera);
    void setScene(const shared<const Scene>& scene);

    void softReset();
    void hardReset();
    void updateInteractive(double timeQuantum);

    string stageName() const;
    double stageProgress() const;
    size_t numRays() const;
    double renderTime() const;
    double raysPerSecond() const;

    const vector<vec4>& image() const;
private:
    enum _Stage {
        _Scatter,
        _Build,
        _BuildDone,
        _Gather
    };

    const size_t _numPhotons;
    _Stage _stage = _Scatter;
    size_t _width = 0;
    size_t _height = 0;
    shared<const Camera> _camera;
    shared<const Scene> _scene;
    vector<vec4> _image;

    vector<Photon> _scattered;
    PhotonMap _photons;

    PhotonMapping(const PhotonMapping&) = delete;
    PhotonMapping& operator=(const PhotonMapping&) = delete;
};

}
