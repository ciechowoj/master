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
        const shared<const Scene>& scene,
        bool unbiased,
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
    string id() const override;

private:
    struct LightVertex {
        SurfacePoint surface;
        vec3 omega;
        vec3 throughput;
        float specular;
        float a, A, b, B;
        float bGeometry;
        float length;

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
        float bGeometry;
        float length;
    };

    static const size_t _maxSubpath = 1024;
    using light_path_t = fixed_vector<LightVertex, _maxSubpath>;

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
        const vec3& omega,
        const SurfacePoint& surface,
        const BSDFQuery& bsdf,
        const vec3& target,
        const Edge& edge);

    vec3 _connect(
        const LightVertex& light, const BSDFQuery& light_bsdf,
        const EyeVertex& eye, const BSDFQuery& eye_bsdf, const Edge& edge);

    vec3 _connect_light(const EyeVertex& eye);

    template <bool SkipDirectVM>
    vec3 _connect(const LightVertex& light, const EyeVertex& eye);

    vec3 _connect(render_context_t& context, const EyeVertex& eye);
    vec3 _connect_eye(render_context_t& context, const EyeVertex& eye);

    vec3 _gather_eye(render_context_t& context, const EyeVertex& eye);

    void _scatter(random_generator_t& generator);

    vec3 _gather(random_generator_t& generator, const EyeVertex& eye, const BSDFQuery& eye_bsdf, const EyeVertex& tentative);

    vec3 _merge(
        random_generator_t& generator,
        const LightVertex& light0,
        const LightVertex& light1,
        const EyeVertex& eye0,
        const EyeVertex& eye1);

    vec3 _merge_light(
        random_generator_t& generator,
        const LightVertex& light,
        const EyeVertex& eye);

    vec3 _merge_eye(
        random_generator_t& generator,
        const LightVertex& light,
        const EyeVertex& eye);

    vec3 _combine(vec3 throughput, float weight);

    bool _russian_roulette(random_generator_t& generator) const;

    const size_t _num_photons;
    const bool _unbiased;
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

using UPG0 = UPGBase<FixedBeta<0>>;
using UPG1 = UPGBase<FixedBeta<1>>;
using UPG2 = UPGBase<FixedBeta<2>>;

class UPGb : public UPGBase<VariableBeta> {
public:
    UPGb(
        const shared<const Scene>& scene,
        bool unbiased,
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
