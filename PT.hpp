#pragma once
#include <Technique.hpp>

namespace haste {

class PathTracing : public Technique {
 public:
  PathTracing(const shared<const Scene>& scene, float lights, float roulette,
              float beta, size_t max_path, size_t num_threads);

  vec3 _traceEye(render_context_t& context, Ray ray) override;

 private:
  struct EyeVertex {
    SurfacePoint surface;
    vec3 omega;
    vec3 throughput;
    float density;
    uint16_t finite;
  };

  vec3 _connect(render_context_t& context, const EyeVertex& eye);

  const size_t _min_subpath = 3;
  const size_t _max_path;
  const float _lights;
  const float _roulette;
  const float _beta;
};
}
