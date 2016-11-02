#pragma once
#include <fixed_vector.hpp>
#include <Technique.hpp>
#include <HashGrid3D.hpp>
#include <Beta.hpp>

namespace haste {

struct Edge;

template <class Beta> class UPGBase : public Technique, protected Beta {
public:
    UPGBase(
        size_t minSubpath,
        float roulette,
        size_t numPhotons,
        size_t numGather);







    string name() const override;

private:
    struct LightVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float specular;
        float a, A, B;
    };

    struct EyeVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float specular;
        float c, C;
    };

    static const size_t _maxSubpath = 1024;

    static void _updateWeights(
        EyeVertex& current,
        const EyeVertex& previous,
        const BSDFSample& bsdf,
        const Edge& edge);

    static void _updateWeights(
        LightVertex& current,
        const LightVertex& previous,
        const BSDFSample& bsdf,
        const Edge& edge);

    vec3 _traceEye(RandomEngine& engine, const Ray& ray) override;

    template <class Appender>
    void _traceLight(RandomEngine& engine, Appender& path);
    void _traceLight(RandomEngine& engine, vector<LightVertex>& path);
    void _traceLight(
        RandomEngine& engine,
        fixed_vector<LightVertex, _maxSubpath>& path);

    vec3 _connect0(RandomEngine& engine, const EyeVertex& eye);
    vec3 _connect1(RandomEngine& engine, const EyeVertex& eye);
    vec3 _connect(const LightVertex& light, const EyeVertex& eye);
    vec3 _connect(
        RandomEngine& engine,
        const EyeVertex& eye,
        const fixed_vector<LightVertex, _maxSubpath>& path);

    void _scatter(RandomEngine& engine);

    vec3 _gather(RandomEngine& engine, const EyeVertex& eye);
    vec3 _merge(const LightVertex& light, const EyeVertex& eye, float radius);

    const size_t _numPhotons;
    const size_t _numGather;
    const size_t _minSubpath;
    const float _roulette;
    const float _eta = 1.0f;
};

typedef UPGBase<FixedBeta<0>> UPG0;
typedef UPGBase<FixedBeta<1>> UPG1;
typedef UPGBase<FixedBeta<2>> UPG2;

class UPGb : public UPGBase<VariableBeta> {
public:
    UPGb(
        size_t minSubpath,
        float roulette,
        size_t numPhotons,
        size_t numGather,
        float beta);
};

}
