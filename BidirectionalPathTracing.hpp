#pragma once
#include <Technique.hpp>

namespace haste {

class BidirectionalPathTracing : public Technique {
private:
	struct Vertex;

public:
    BidirectionalPathTracing();

     void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId) override;

    vec3 trace(RandomEngine& engine, Ray ray);
    size_t traceLightSubpath(Vertex* subpath);
    size_t traceEyeSubpath(Vertex* subpath, Ray ray);

    string name() const override;

private:
	struct Vertex {
		vec3 throughput;
		float density;
	};

	static const size_t _maxSubpathLength = 64;

    vec3 _pathtrace(RandomEngine& engine, Ray ray);

    double _progress;
};

}
