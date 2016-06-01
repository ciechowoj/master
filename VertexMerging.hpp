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
    struct LightVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float density;
        float b, B, C;

        float operator[](size_t i) const { return surface.position()[i]; }

        vec3 position() const { return surface.position(); }
        vec3 normal() const { return surface.normal(); }
    };

    struct EyeVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float density;
        float specular;
        float d, D;

        vec3 position() const { return surface.position(); }
        vec3 normal() const { return surface.normal(); }
    };

    KDTree3D<LightVertex> _vertices;

    size_t _numPhotons;
    size_t _numGather;
    float _maxRadius;

    float _eta() const;
    vec3 _gather(const EyeVertex* eye, RandomEngine& engine);
    vec3 _merge(const EyeVertex* eye, const LightVertex* light);

    vec3 _connect(
        RandomEngine& engine,
        const EyeVertex& eye,
        const LightVertex* lightPath,
        size_t lightSize);

    vec3 _connect(const EyeVertex& eye, const LightVertex& light);
    size_t _trace(LightVertex* path, RandomEngine& engine);
    vec3 _trace(RandomEngine& engine, const Ray& ray);
    void _scatter(RandomEngine& engine);
};

}
