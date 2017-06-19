#pragma once
#include <Beta.hpp>
#include <HashGrid3D.hpp>
#include <Technique.hpp>
#include <fixed_vector.hpp>

namespace haste {

struct Edge;

template <class Beta>
class UPGBase : public Technique, protected Beta {
 public:
  UPGBase(const shared<const Scene>& scene, bool unbiased, bool enable_vc,
          bool enable_vm, bool from_light, float lights, float roulette, size_t numPhotons,
          float radius, float alpha, float beta, size_t numThreads);

 private:
  struct LightVertex {
    SurfacePoint surface;
    vec3 omega;
    vec3 throughput;
    float a, A, B;
    float bGeometry;
    uint16_t length : 13;
    uint16_t directional : 14;
    uint16_t finite : 1;

    const vec3& position() const { return surface.position(); }

    bool is_light() const { return surface.is_light(); }
  };

  struct EyeVertex {
    SurfacePoint surface;
    vec3 omega;
    vec3 throughput;
    float bGeometry;
    float c, C, D;
    uint16_t length : 15;
    uint16_t finite : 1;
  };

  static const size_t _maxSubpath = 1024;
  using light_path_t = fixed_vector<LightVertex, _maxSubpath>;

  vec3 _traceEye(render_context_t& context, Ray ray) override;
  void _preprocess(random_generator_t& generator, double num_samples) override;

  LightVertex _sample_to_vertex(const LightSample& sample);
  LightVertex _sample_light(random_generator_t& generator);

  void _traceLight(random_generator_t& generator, vector<LightVertex>& path,
                   size_t& size);

  float _vc_subweight_inv(const LightVertex& light, const BSDFQuery& lightBSDF,
                          const EyeVertex& eye, const BSDFQuery& eyeBSDF,
                          const Edge& edge);

  float _vm_subweight_inv(const LightVertex& light, const BSDFQuery& lightBSDF,
                          const EyeVertex& eye, const BSDFQuery& eyeBSDF,
                          const Edge& edge);

  float _vm_biased_subweight_inv(const LightVertex& light,
                                 const BSDFQuery& lightBSDF,
                                 const EyeVertex& eye, const BSDFQuery& eyeBSDF,
                                 const Edge& edge, float vm_current);

  float _vc_weight(const LightVertex& light, const BSDFQuery& lightBSDF,
                   const EyeVertex& eye, const BSDFQuery& eyeBSDF,
                   const Edge& edge);

  float _vc_biased_weight(const LightVertex& light, const BSDFQuery& lightBSDF,
                          const EyeVertex& eye, const BSDFQuery& eyeBSDF,
                          const Edge& edge, float vm_current);

  float _vm_biased_weight(const LightVertex& light,
                          const BSDFQuery& lightBSDF, const EyeVertex& eye,
                          const BSDFQuery& eyeBSDF, const Edge& edge,
                          float vm_current);

  float _weight_vm_eye(const LightVertex& light, const BSDFQuery& lightBSDF,
                       const EyeVertex& eye, const BSDFQuery& eyeBSDF,
                       const Edge& edge);

  float _weight_vm_light(const LightVertex& light, const BSDFQuery& lightBSDF,
                         const EyeVertex& eye, const BSDFQuery& eyeBSDF,
                         const Edge& edge);

  float _density(random_generator_t& generator, const vec3& omega,
                 const SurfacePoint& surface, const vec3& target);

  vec3 _connect(const LightVertex& light, const BSDFQuery& light_bsdf,
                const EyeVertex& eye, const BSDFQuery& eye_bsdf,
                const Edge& edge);

  vec3 _connect_light(const EyeVertex& eye);
  vec3 _connect_directional(const EyeVertex& eye, const LightSample& sample);

  vec3 _connect(const LightVertex& light, const EyeVertex& eye);

  vec3 _connect(render_context_t& context, const EyeVertex& eye);

  void _scatter(random_generator_t& generator);

  vec3 _gather(render_context_t& context, const EyeVertex& eye,
               const EyeVertex& tentative);

  vec3 _gather_biased(render_context_t& context, const EyeVertex& eye,
                      const EyeVertex& tentative);

  vec3 _merge_light(random_generator_t& generator, const LightVertex& light,
                    const EyeVertex& eye);

  vec3 _merge_eye(random_generator_t& generator, const LightVertex& light,
                  const EyeVertex& eye);

  vec3 _merge_biased(random_generator_t& generator, const LightVertex& light,
                     const LightVertex& tentative, const EyeVertex& eye);

  float _clamp(float x) const;

  bool _russian_roulette(random_generator_t& generator) const;

  static const bool _merge_from_light = true;
  static const int _trim_light = _merge_from_light ? 1 : 0;
  static const int _trim_eye = _merge_from_light ? 1 : 0;

  const size_t _num_photons;
  const bool _unbiased;
  const bool _enable_vc;
  const bool _enable_vm;
  const bool _from_light;
  const float _lights;
  const float _roulette;
  const float _roulette_inv;
  const float _initial_radius;
  const float _alpha;
  const float _clamp_const;

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
  UPGb(const shared<const Scene>& scene, bool unbiased, bool enable_vc,
       bool enable_vm, bool from_light, float lights, float roulette, 
       size_t numPhotons, float radius, float alpha, float beta, 
       size_t numThreads);
};

}
