#pragma once
#include <Camera.hpp>
#include <Scene.hpp>

namespace haste {

class Technique {
public:
    Technique();
    virtual ~Technique();

    void setCamera(const shared<const Camera>& camera);
    void setScene(const shared<const Scene>& scene);

    virtual void softReset();
    virtual void hardReset();
    virtual void updateInteractive(
        size_t width,
        size_t height,
        vec4* image,
        double timeQuantum) = 0;

    virtual string stageName() const = 0;
    virtual double stageProgress() const;
    size_t numRays() const;
    double renderTime() const;
    double raysPerSecond() const;
    size_t numSamples() const;
protected:
    shared<const Camera> _camera;
    shared<const Scene> _scene;

    size_t _numRays = 0;
    size_t _numSamples = 0;
    double _renderTime = 0.0;

private:
    Technique(const Technique&) = delete;
    Technique& operator=(const Technique&) = delete;
};

}
