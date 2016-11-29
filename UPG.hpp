#pragma once
#include <fixed_vector.hpp>
#include <Technique.hpp>
#include <HashGrid3D.hpp>
#include <Beta.hpp>

namespace haste {

struct Edge;

enum class GatherMode {
    Unbiased,
    Biased,
};

template <class Beta, GatherMode Mode> class UPGBase : public Technique, protected Beta {
public:
    UPGBase(
        size_t minSubpath,
        float roulette,
        size_t numPhotons,
        size_t numGather,
        float radius,
        size_t numThreads);

    void preprocess(
        const shared<const Scene>& scene,
        RandomEngine& engine,
        const function<void(string, float)>& progress,
        bool parallel) override;

    string name() const override;

private:
    struct LightVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float specular;
        float a, A, B;

        const vec3& position() const {
            return surface.position();
        }
    };

    struct EyeVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float specular;
        float c, C, d, D;
    };

    static const size_t _maxSubpath = 1024;
    using light_path_t = fixed_vector<LightVertex, _maxSubpath>;

    vec3 _traceEye(render_context_t& context, Ray ray) override;

    template <bool First, class Appender>
    void _traceLight(RandomEngine& engine, Appender& path);
    void _traceLight(RandomEngine& engine, vector<LightVertex>& path);
    void _traceLight(
        RandomEngine& engine,
        light_path_t& path);

    template <bool SkipDirectVM> float _weightVC(
        const LightVertex& light,
        const BSDFQuery& lightBSDF,
        const EyeVertex& eye,
        const BSDFQuery& eyeBSDF,
        const Edge& edge,
        float radius);

    float _weightVM(
        const LightVertex& light,
        const BSDFQuery& lightBSDF,
        const EyeVertex& eye,
        const BSDFQuery& eyeBSDF,
        const Edge& edge,
        float radius);

    float _density(
        RandomEngine& engine,
        const LightVertex& light,
        const EyeVertex& eye,
        const BSDFQuery& eyeQuery,
        const Edge& edge,
        float radius);

    vec3 _connect_light(const EyeVertex& eye, float radius);

    template <bool SkipDirectVM>
    vec3 _connect(const LightVertex& light, const EyeVertex& eye, float radius);

    vec3 _connect(
        const EyeVertex& eye,
        const light_path_t& path,
        float radius);

    vec3 _connect_eye(render_context_t& context, const EyeVertex& eye, const light_path_t& path, float radius);

    void _scatter(RandomEngine& engine);

    vec3 _gather(RandomEngine& engine, const EyeVertex& eye, float& radius);

    vec3 _merge(
        RandomEngine& engine,
        const LightVertex& light,
        const EyeVertex& eye,
        float radius);

    vec3 _merge(
        RandomEngine& engine,
        const LightVertex& light,
        const EyeVertex& eye,
        const BSDFQuery& eyeBSDF,
        float radius);

    vec3 _combine(vec3 throughput, float weight);

    const size_t _numPhotons;
    const size_t _numGather;
    const size_t _minSubpath;
    const float _roulette;
    const float _radius;

    v3::HashGrid3D<LightVertex> _vertices;
};

using UPG0 = UPGBase<FixedBeta<0>, GatherMode::Unbiased>;
using UPG1 = UPGBase<FixedBeta<1>, GatherMode::Unbiased>;
using UPG2 = UPGBase<FixedBeta<2>, GatherMode::Unbiased>;

class UPGb : public UPGBase<VariableBeta, GatherMode::Unbiased> {
public:
    UPGb(
        size_t minSubpath,
        float roulette,
        size_t numPhotons,
        size_t numGather,
        float radius,
        float beta,
        size_t numThreads);
};

using VCM0 = UPGBase<FixedBeta<0>, GatherMode::Biased>;
using VCM1 = UPGBase<FixedBeta<1>, GatherMode::Biased>;
using VCM2 = UPGBase<FixedBeta<2>, GatherMode::Biased>;

class VCMb : public UPGBase<VariableBeta, GatherMode::Biased> {
public:
    VCMb(
        size_t minSubpath,
        float roulette,
        size_t numPhotons,
        size_t numGather,
        float radius,
        float beta,
        size_t numThreads);
};


}
