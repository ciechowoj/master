#pragma once
#include <Technique.hpp>
#include <KDTree3D.hpp>

namespace haste {

class VCM : public Technique {
public:
    VCM(size_t numPhotons, size_t numGather, float maxRadius);

    void preprocess(
        const shared<const Scene>& scene,
        RandomEngine& engine,
        const function<void(string, float)>& progress,
        bool parallel);

    void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId) override;

    string name() const override;


private:
    struct Vertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float density;
        float specular;
        float B, b, C;

        float operator[](size_t i) const { return surface.position()[i]; }
    };

    KDTree3D<Vertex> _vertices;

    size_t _numPhotons;
    size_t _numGather;
    float _maxRadius;

    void _traceLightPaths(const Scene* scene, RandomEngine& engine);
};

}
