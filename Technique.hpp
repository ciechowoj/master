#pragma once
#include <Camera.hpp>
#include <Scene.hpp>

namespace haste {

class Technique {
public:
    Technique();
    virtual ~Technique();

    void setImageSize(size_t width, size_t height);
    void setCamera(const shared<const Camera>& camera);
    void setScene(const shared<const Scene>& scene);

    virtual void softReset();
    virtual void hardReset();
    virtual void updateInteractive(double timeQuantum) = 0;

    virtual string stageName() const = 0;
    virtual double stageProgress() const;
    size_t numRays() const;
    double renderTime() const;
    double raysPerSecond() const;
    size_t numSamples() const;

    const vector<vec4>& image() const;

protected:
    size_t _width;
    size_t _height;
    shared<const Camera> _camera;
    shared<const Scene> _scene;
    vector<vec4> _image;

    size_t _numRays = 0;
    double _renderTime = 0.0;

private:
    Technique(const Technique&) = delete;
    Technique& operator=(const Technique&) = delete;
};

}
