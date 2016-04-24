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
    size_t traceLightSubpath(RandomEngine& engine, Vertex* subpath);
    size_t traceEyeSubpath(RandomEngine& engine, Vertex* subpath, Ray ray);
    size_t traceSubpath(RandomEngine& engine, Vertex* subpath, vec3 omega);


    string name() const override;

private:
	struct Vertex {
		SurfacePoint _point;
        vec3 _omega;
		vec3 _throughput;
		float _density;
        const BSDF* _bsdf;

        const SurfacePoint& point() const { return _point; }
        const vec3& omega() const { return _omega; }
		const vec3& position() const { return _point.position(); }
        const vec3& normal() const { return _point.normal(); }
		const vec3& throughput() const { return _throughput; }
		const float density() const { return _density; }
        const BSDF* bsdf() const { return _bsdf; }
	};

	static const size_t _maxSubpathLength = 64;
	static const size_t _russianRouletteInv1K = 1052; // (0.95)^-1 * 1000
    vec3 _pathtrace(RandomEngine& engine, Ray ray);

    double _progress;
};

}
