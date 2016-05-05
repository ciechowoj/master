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

    vec3 trace(RandomEngine& engine, Ray ray, const BSDF* cameraBSDF);
    pair<size_t, vec3> traceLightSubpath(RandomEngine& engine, Vertex* subpath);
    pair<size_t, vec3> traceEyeSubpath(RandomEngine& engine, Vertex* subpath, Ray ray, const BSDF* cameraBSDF);
    pair<size_t, vec3> traceSubpath(RandomEngine& engine, Vertex* subpath, vec3 omega, size_t size);


    string name() const override;

private:
    struct Vertex {
        SurfacePoint _point;
        vec3 _omega;
        vec3 _throughput;
        float _specular;
        float _density;
        const BSDF* _bsdf;
        float _weightC;
        float _weightD;

        const SurfacePoint& point() const { return _point; }
        const vec3& omega() const { return _omega; }
        const vec3& position() const { return _point.position(); }
        const vec3& normal() const { return _point.normal(); }
        const vec3& gnormal() const { return _point.gnormal(); }
        const vec3& throughput() const { return _throughput; }
        const float specular() const { return _specular; }
        const float density() const { return _density; }
        const BSDF* bsdf() const { return _bsdf; }
        const float weightC() const { return _weightC; }
        const float weightD() const { return _weightD; }
    };

    static const size_t _maxSubpathLength = 64;
    static const size_t _russianRouletteInv1K = 2000; //1052; // (0.95)^-1 * 1000
    vec3 _pathtrace(RandomEngine& engine, Ray ray);

    double _progress;
};

}
