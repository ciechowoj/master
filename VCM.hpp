#pragma once
#include <Technique.hpp>
#include <KDTree3D.hpp>

namespace haste {

class VCM : public Technique {
public:
    VCM(
        size_t numPhotons = 10000,
        size_t numGather = 100,
        float maxRadius = 0.1f,
        size_t minSubpath = 3,
        float roulette = 0.5f);

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
        float a, A, B;

        float operator[](size_t i) const { return surface.position()[i]; }

        const vec3& position() const { return surface.position(); }
        const vec3& normal() const { return surface.normal(); }
        const vec3& gnormal() const { return surface.gnormal(); }
    };

    struct EyeVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float specular;
        float c, C;

        const vec3& position() const { return surface.position(); }
        const vec3& normal() const { return surface.normal(); }
        const vec3& gnormal() const { return surface.gnormal(); }
    };

    struct Edge
    {
        template <class A, class B> Edge(const A& fst, const B& snd)
        {
            distSqInv = 1.0f / distance2(fst.position(), snd.position());
            fCosTheta = abs(dot(fst.omega(), snd.gnormal()));
            bCosTheta = abs(dot(fst.omega(), fst.gnormal()));
            fGeometry = distSqInv * fCosTheta;
            bGeometry = distSqInv * bCosTheta;
        }

        template <class A, class B> Edge(const A& fst, const B& snd, const vec3& omega)
        {
            distSqInv = 1.0f / distance2(fst.position(), snd.position());
            fCosTheta = abs(dot(omega, snd.gnormal()));
            bCosTheta = abs(dot(omega, fst.gnormal()));
            fGeometry = distSqInv * fCosTheta;
            bGeometry = distSqInv * bCosTheta;
        }

        float distSqInv;
        float fCosTheta;
        float bCosTheta;
        float fGeometry;
        float bGeometry;
    };

    static const size_t _maxSubpath = 128;
    const size_t _numPhotons;
    const size_t _numGather;
    const float _maxRadius;
    const size_t _minSubpath;
    const float _roulette;
    const float _eta;

    KDTree3D<LightVertex> _vertices;

    void _trace(RandomEngine& engine, size_t& size, LightVertex* path);
    vec3 _trace(RandomEngine& engine, const Ray& ray);
    vec3 _connect0(RandomEngine& engine, const EyeVertex& eye);
    vec3 _connect1(RandomEngine& engine, const EyeVertex& eye);
    vec3 _connect(const EyeVertex& eye, const LightVertex& light);
    vec3 _connect(RandomEngine& engine, const EyeVertex& eye, size_t size, const LightVertex* path);

    void _scatter(RandomEngine& engine);
    vec3 _gather(RandomEngine& engine, const EyeVertex& eye);
    vec3 _merge(const EyeVertex& eye, const LightVertex& light, float tdensity);
};

}
