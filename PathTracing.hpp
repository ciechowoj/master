#pragma once
#include <Camera.hpp>
#include <Scene.hpp>

namespace haste {

class PathTracing {
public:
	PathTracing();

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
	size_t _width;
	size_t _height;
	shared<const Camera> _camera;
	shared<const Scene> _scene;
	vector<vec4> _image;

	size_t _numRays = 0;
	double _renderTime = 0.0;

	PathTracing(const PathTracing&) = delete;
	PathTracing& operator=(const PathTracing&) = delete;
};

}
