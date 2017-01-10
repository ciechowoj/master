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
        const shared<const Scene>& scene,
        bool enable_vc,
        bool enable_vm,
        float lights,
        float roulette,
        size_t numPhotons,
        float radius,
        float alpha,
        float beta,
        size_t numThreads);

    string name() const override;

private:
    struct LightVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float specular;
        float a, A, b, B;
        float bGeometry;
        int length;

        const vec3& position() const {
            return surface.position();
        }

        bool is_light() const {
            return surface.is_light();
        }
    };

    struct EyeVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float specular;
        float c, C, d, D;
        int length;
    };

    static const size_t _maxSubpath = 1024;
    using light_path_t = fixed_vector<LightVertex, _maxSubpath>;

    float clamp(float x) const;
    float beta(float x) const;

    vec3 _traceEye(render_context_t& context, Ray ray) override;
    void _preprocess(random_generator_t& generator, double num_samples) override;

    LightVertex _sample_light(random_generator_t& generator);

    void _traceLight(random_generator_t& generator, vector<LightVertex>& path, size_t& size);

    template <bool SkipDirectVM> float _weightVC(
        const LightVertex& light,
        const BSDFQuery& lightBSDF,
        const EyeVertex& eye,
        const BSDFQuery& eyeBSDF,
        const Edge& edge);

    float _weightVM(
        const LightVertex& light,
        const BSDFQuery& lightBSDF,
        const EyeVertex& eye,
        const BSDFQuery& eyeBSDF,
        const Edge& edge);

    float _density(
        random_generator_t& generator,
        const SurfacePoint& from,
        vec3 omega,
        const SurfacePoint& target);

    vec3 _connect_light(const EyeVertex& eye);

    template <bool SkipDirectVM>
    vec3 _connect(const LightVertex& light, const EyeVertex& eye);

    vec3 _connect(random_generator_t& generator, const EyeVertex& eye, const std::size_t& tail);

    vec3 _connect_eye(render_context_t& context, const EyeVertex& eye, const std::size_t& path);
    vec3 _gather_eye(render_context_t& context, const EyeVertex& eye);

    void _scatter(random_generator_t& generator);

    vec3 _gather(random_generator_t& generator, const EyeVertex& eye, const BSDFQuery& eye_bsdf, const EyeVertex& tentative);

    vec3 _merge(
        random_generator_t& generator,
        const EyeVertex& eye_fst,
        const EyeVertex& eye_snd,
        const LightVertex& light_fst,
        const LightVertex& light_snd);

    vec3 _merge(
        random_generator_t& generator,
        const EyeVertex& eye,
        const LightVertex& light);

    vec3 _merge(
        random_generator_t& generator,
        const LightVertex& light,
        const EyeVertex& eye,
        const BSDFQuery& eyeBSDF);

    vec3 _combine(vec3 throughput, float weight);

    bool _russian_roulette(random_generator_t& generator) const;

    const size_t _num_photons;
    const bool _enable_vc;
    const bool _enable_vm;
    const float _lights;
    const float _roulette;
    const float _initial_radius;
    const float _alpha;

    size_t _num_scattered;
    float _num_scattered_inv;
    float _radius;
    float _circle;

    vector<LightVertex> _light_paths;
    vector<uint32_t> _light_offsets;
    v3::HashGrid3D<LightVertex> _vertices;
};

using UPG0 = UPGBase<FixedBeta<0>, GatherMode::Unbiased>;
using UPG1 = UPGBase<FixedBeta<1>, GatherMode::Unbiased>;
using UPG2 = UPGBase<FixedBeta<2>, GatherMode::Unbiased>;

class UPGb : public UPGBase<VariableBeta, GatherMode::Unbiased> {
public:
    UPGb(
        const shared<const Scene>& scene,
        bool enable_vc,
        bool enable_vm,
        float lights,
        float roulette,
        size_t numPhotons,
        float radius,
        float alpha,
        float beta,
        size_t numThreads);
};

using VCM0 = UPGBase<FixedBeta<0>, GatherMode::Biased>;
using VCM1 = UPGBase<FixedBeta<1>, GatherMode::Biased>;
using VCM2 = UPGBase<FixedBeta<2>, GatherMode::Biased>;

class VCMb : public UPGBase<VariableBeta, GatherMode::Biased> {
public:
    VCMb(
        const shared<const Scene>& scene,
        bool enable_vc,
        bool enable_vm,
        float lights,
        float roulette,
        size_t numPhotons,
        float radius,
        float alpha,
        float beta,
        size_t numThreads);
};


}
