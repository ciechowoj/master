#include <Edge.hpp>
#include <PT.hpp>

namespace haste {

PathTracing::PathTracing(size_t min_subpath, float roulette, float beta,
                         size_t max_path, size_t num_threads)
    : Technique(num_threads),
      _min_subpath(min_subpath),
      _max_path(max_path),
      _roulette(roulette),
      _beta(beta) {}

vec3 PathTracing::_traceEye(render_context_t& context, Ray ray) {
  vec3 radiance = vec3(0.0f);
  EyeVertex eye[2];
  size_t itr = 0, prv = 1;

  RayIsect isect = _scene->intersect(ray.origin, ray.direction);

  while (isect.isLight() && _max_path > 0) {
    radiance += _scene->queryRadiance(isect, -ray.direction);
    isect = _scene->intersect(isect.position(), ray.direction);
  }

  if (!isect.isPresent() || _max_path < 2) {
    return radiance;
  }

  eye[prv].surface = _scene->querySurface(isect);
  eye[prv].omega = -ray.direction;
  eye[prv].throughput = vec3(1.0f);
  eye[prv].specular = 0.0f;
  eye[prv].density = 1.0f;

  size_t path_size = 2;

  while (path_size <= _max_path) {
    radiance += _connect(context, eye[prv]);

    auto bsdf =
        _scene->sampleBSDF(*context.engine, eye[prv].surface, eye[prv].omega);

    while (true) {
      isect = _scene->intersect(isect.position(), bsdf.omega);

      if (!isect.isPresent()) {
        return radiance;
      }

      eye[itr].surface = _scene->querySurface(isect);
      eye[itr].omega = -bsdf.omega;

      auto edge = Edge(eye[prv], eye[itr]);

      eye[itr].throughput =
          eye[prv].throughput * bsdf.throughput * edge.bCosTheta / bsdf.density;

      eye[prv].specular = bsdf.specular;
      eye[itr].density = eye[prv].density * edge.fGeometry * bsdf.density;

      if (isect.isLight()) {
        auto lsdf = _scene->queryLSDF(eye[itr].surface, eye[itr].omega);
        float weightInv = pow(lsdf.areaDensity(), _beta) /
                              pow(edge.fGeometry * bsdf.density, _beta) +
                          1.0f;

        if (bsdf.specular == 1.0f) weightInv = 1.0f;

        radiance += lsdf.radiance() * eye[itr].throughput / weightInv;
      } else {
        break;
      }
    }

    std::swap(itr, prv);

    float roulette = path_size < _min_subpath ? 1.0f : _roulette;
    float uniform = sampleUniform1(*context.engine).value();

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
  LightSample light = _scene->sampleLight(*context.engine);
  vec3 omega = normalize(eye.surface.position() - light.position());

  if (dot(omega, light.normal()) < 0.0f) {
    return vec3(0.0f);
  }

  auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

  auto edge = Edge(light, eye, omega);

  float weightInv = pow(eyeBSDF.densityRev * edge.bGeometry, _beta) /
                        pow(light.areaDensity(), _beta) +
                    1.0f;

  return _scene->occluded(eye.surface.position(), light.position()) *
         light.radiance() / light.areaDensity() * eye.throughput *
         eyeBSDF.throughput * edge.bCosTheta * edge.fGeometry / weightInv;
}

string PathTracing::name() const { return "Path Tracing"; }
}
