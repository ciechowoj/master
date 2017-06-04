#include <PT.hpp>

namespace haste {

PathTracing::PathTracing(const shared<const Scene>& scene,
                         float lights, float roulette, float beta,
                         size_t max_path, size_t num_threads)
    : Technique(scene, num_threads),
      _max_path(max_path),
      _lights(lights),
      _roulette(roulette),
      _beta(beta) {
}

vec3 PathTracing::_traceEye(render_context_t& context, Ray ray) {
  vec3 radiance = vec3(0.0f);
  EyeVertex eye[2];
  size_t itr = 0, prv = 1;

  SurfacePoint surface =
      _scene->intersect(_camera_surface(context), ray.direction);

  while (surface.is_light() && _max_path > 0) {
    radiance += _lights * _scene->queryLSDF(surface, -ray.direction).radiance;
    surface = _scene->intersect(surface, ray.direction);
  }

  if (!surface.is_present() || _max_path < 2) {
    return radiance;
  }

  eye[prv].surface = surface;
  eye[prv].omega = -ray.direction;
  eye[prv].throughput = vec3(1.0f);
  eye[prv].finite = 1;
  eye[prv].density = 1.0f;

  size_t path_size = 2;

  while (path_size <= _max_path) {
    radiance += _connect(context, eye[prv]);

    auto bsdf =
        _scene->sampleBSDF(*context.generator, eye[prv].surface, eye[prv].omega);

    while (true) {
      surface = _scene->intersect(surface, bsdf.omega);

      if (!surface.is_present()) {
        return radiance;
      }

      eye[itr].surface = surface;
      eye[itr].omega = -bsdf.omega;

      auto edge = Edge(eye[prv].surface, eye[itr].surface, eye[itr].omega);

      eye[itr].throughput =
          eye[prv].throughput * bsdf.throughput * edge.bCosTheta;

      if (l1Norm(eye[itr].throughput) < FLT_EPSILON) {
        return radiance;
      }

      eye[itr].throughput /= bsdf.density;

      eye[prv].finite = bsdf.finite;
      eye[itr].density = eye[prv].density * edge.fGeometry * bsdf.density;

      if (surface.is_light()) {
        auto lsdf = _scene->queryLSDF(eye[itr].surface, eye[itr].omega);
        float weightInv = pow(lsdf.density, _beta) /
                              pow(edge.fGeometry * bsdf.density, _beta) +
                          1.0f;

        if (bsdf.finite == 0) weightInv = 1.0f;

        radiance += lsdf.radiance * eye[itr].throughput / weightInv;
      } else {
        break;
      }
    }

    std::swap(itr, prv);

    float roulette = path_size < _min_subpath ? 1.0f : _roulette;
    float uniform = context.generator->sample();

    if (roulette < uniform) {
      return radiance;
    } else {
      eye[prv].throughput /= roulette;
      ++path_size;
    }
  }

  return radiance;
}

vec3 PathTracing::_connect(render_context_t& context, const EyeVertex& eye) {
  LightSample light = _scene->sampleLight(*context.generator);
  vec3 omega = normalize(eye.surface.position() - light.position());
  auto bsdf = _scene->queryBSDF(light.surface, light.normal(), omega);

  if (l1Norm(bsdf.throughput) < FLT_EPSILON) {
    return vec3(0.0f);
  }

  auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

  auto edge = Edge(light.surface, eye.surface, omega);

  float weightInv = pow(eyeBSDF.densityRev * edge.bGeometry, _beta) /
                        pow(light.combined_density(), _beta) +
                    1.0f;

  return _scene->occluded(eye.surface, light.surface) * light.radiance() /
         light.combined_density() * eye.throughput * eyeBSDF.throughput *
         edge.bCosTheta * edge.fGeometry / weightInv;
}

string PathTracing::name() const { return "Path Tracing"; }

string PathTracing::id() const {
    return string("PT_b") + std::to_string(this->_beta);
}

}
