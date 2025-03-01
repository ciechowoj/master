#include <UPG.hpp>
#include <condition_variable>
#include <streamops.hpp>

namespace haste {

template <class Beta>
UPGBase<Beta>::UPGBase(
  const shared<const Scene>& scene,
  bool unbiased,
  bool enable_vc,
  bool enable_vm,
  bool from_light,
  float lights,
  float roulette,
  size_t numPhotons,
  float radius,
  float alpha,
  float beta,
  size_t num_threads)
  : Technique(scene, num_threads)
  , _num_photons(numPhotons)
  , _unbiased(unbiased)
  , _enable_vc(enable_vc)
  , _enable_vm(enable_vm)
  , _from_light(from_light)
  , _lights(lights)
  , _roulette(roulette)
  , _roulette_inv(1.0f / roulette)
  , _initial_radius(radius)
  , _alpha(alpha)
  , _clamp_const(unbiased ? 1.0f : FLT_MAX)
  , _num_scattered(0)
  , _num_scattered_inv(0.0f)
  , _radius(radius)
  , _circle(pi<float>() * radius * radius) {
}

template <class Beta>
vec3 UPGBase<Beta>::_traceEye(render_context_t& context, Ray ray) {
  time_scope_t _0(_statistics.trace_eye_time);

  vec3 radiance = vec3(0.0f);

  if (_russian_roulette(*context.generator)) {
    return radiance;
  }

  EyeVertex eye[2];
  auto prv = &eye[0];
  auto itr = &eye[1];

  SurfacePoint surface = _camera_surface(context);
  prv->surface = surface;
  prv->omega = -ray.direction;
  prv->throughput = vec3(1.0f) * _roulette_inv;
  prv->finite = 1;
  prv->c = 0;
  prv->C = 0;
  prv->D = 0;
  prv->length = 0;

  BSDFSample bsdf = _scene->sampleBSDF(*context.generator, prv->surface, prv->omega);
  BSDFSample new_bsdf;

  while (true) {
    if (_enable_vc) {
      radiance += _connect(context, *prv);
    }

    while (true) {
      surface = _scene->intersect(surface, bsdf.omega);

      if (!surface.is_present()) {
        if (prv->surface.is_camera()) {
          return sky_gradient(bsdf.omega) *= _roulette_inv;
        }

        return radiance;
      }

      itr->surface = surface;
      itr->omega = -bsdf.omega;


      new_bsdf = _scene->sampleBSDF(*context.generator, itr->surface, itr->omega);

      auto edge = Edge(prv->surface, itr->surface, itr->omega);

      itr->throughput
        = prv->throughput
        * bsdf.throughput
        * edge.bCosTheta;

      if (l1Norm(itr->throughput) < FLT_EPSILON) {
        return radiance;
      }

      itr->throughput /= bsdf.density;

      itr->bGeometry = edge.bGeometry;
      itr->length = prv->length + 1;

      prv->finite = min(prv->finite, bsdf.finite);
      itr->finite = bsdf.finite;
      itr->c = 1.0f / (edge.fGeometry * bsdf.density);

      itr->C
        = (prv->C
          * Beta::beta(bsdf.densityRev)
          + prv->finite * Beta::beta(prv->c))
        * Beta::beta(edge.bGeometry * itr->c);

      float vertex_merging = _from_light
        ? _clamp(Beta::beta(_circle * prv->bGeometry * bsdf.densityRev))
        * (prv->length <= 1.0f ? 0.0f : 1.0f)
        : _clamp(Beta::beta(_circle / prv->c));

      itr->D
        = (prv->D
          * Beta::beta(bsdf.densityRev)
          + prv->finite
          * vertex_merging
          * (prv->length <= _trim_eye ? 0.0f : 1.0f)
          * Beta::beta(prv->c))
        * Beta::beta(edge.bGeometry * itr->c);

      if (surface.is_light()) {
        if (_enable_vc) {
          auto lsdf = _scene->queryLSDF(itr->surface, itr->omega);
          auto camera_bsdf = _scene->queryBSDF(itr->surface, vec3(0.0f), itr->omega);

          float Dp
            = (prv->D
              * Beta::beta(bsdf.densityRev)
              + prv->finite
              * vertex_merging
              * (prv->length <= _trim_eye ? 0.0f : 1.0f)
              * Beta::beta(prv->c))
            * Beta::beta(edge.bGeometry * camera_bsdf.density);

          Dp *= (itr->length <= 2.0f ? 0.0f : 1.0f) * float(_enable_vm);
          Dp *= Beta::beta(lsdf.density * itr->c);

          radiance += _connect_light(*itr, Dp);
        }
      }
      else {
        bsdf = new_bsdf;
        break;
      }
    }

    itr->throughput *= _roulette_inv;

    if (_enable_vm) {
      time_scope_t _2(_statistics.gather_time);
      if (_unbiased) {
        radiance += _gather(context, *prv, *itr);
      }
      else {
        radiance += _gather_biased(context, *prv, *itr);
      }
    }

    std::swap(itr, prv);

    if (_russian_roulette(*context.generator)) {
      return radiance;
    }
  }

  return radiance;
}

template <class Beta>
void UPGBase<Beta>::_preprocess(random_generator_t& generator, double num_samples) {
  time_scope_t _0(_statistics.scatter_time);

  if (!_unbiased) {
      _radius = _initial_radius * pow((num_samples + 1.0f), _alpha * 0.5f - 0.5f);
      _circle = pi<float>() * _radius * _radius;
  }

  _scatter(generator);
}

template <class Beta>
typename UPGBase<Beta>::LightVertex UPGBase<Beta>::_sample_to_vertex(const LightSample& sample) {
  LightVertex vertex;
  vertex.surface = sample.surface;
  vertex.omega = vertex.surface.normal();
  vertex.throughput = sample.radiance() / sample.combined_density() * _roulette_inv;
  vertex.a = sample.kind == light_kind::directional ? 0.0f : 1.0f / sample.combined_density();
  vertex.A = 0.0f;
  vertex.B = 0.0f;
  vertex.length = 0;
  vertex.directional = sample.kind == light_kind::directional;
  vertex.finite = 1;

  return vertex;
}

template <class Beta>
typename UPGBase<Beta>::LightVertex UPGBase<Beta>::_sample_light(random_generator_t& generator) {
  return _sample_to_vertex(_scene->sampleLight(generator));
}

template <class Beta>
void UPGBase<Beta>::_traceLight(random_generator_t& generator, vector<LightVertex>& path, size_t& size) {
  if (_russian_roulette(generator)) {
    return;
  }

  path.resize(std::max(path.size(), size + _maxSubpath));

  auto begin = path.data() + size;
  auto prv = begin;
  auto itr = begin + 1;

  *prv = _sample_light(generator);

  BSDFSample bsdf = _scene->sampleBSDF(generator, prv->surface, prv->omega);
  BSDFSample new_bsdf;

  while (!_russian_roulette(generator)) {
    auto surface = _scene->intersectMesh(prv->surface, bsdf.omega);

    if (!surface.is_present()) {
      break;
    }

    itr->surface = surface;
    itr->omega = -bsdf.omega;

    new_bsdf = _scene->sampleBSDF(generator, itr->surface, itr->omega);

    auto edge = Edge(prv->surface, itr->surface, itr->omega);

    itr->throughput
      = prv->throughput
      * bsdf.throughput
      * edge.bCosTheta
      * _roulette_inv;

    if (l1Norm(itr->throughput) < FLT_EPSILON) {
      break;
    }

    itr->throughput /= bsdf.density;

    itr->bGeometry = edge.bGeometry;
    itr->length = prv->length + 1;

    itr->directional = false;
    prv->finite = min(prv->finite, bsdf.finite);
    itr->finite = bsdf.finite;

    itr->a = 1.0f / (edge.fGeometry * bsdf.density);

    itr->A
      = (prv->A
        * Beta::beta(bsdf.densityRev)
        + prv->finite * Beta::beta(prv->a))
      * Beta::beta(edge.bGeometry * itr->a);

    float vertex_merging = _from_light
      ? _clamp(Beta::beta(_circle / prv->a))
      : _clamp(Beta::beta(_circle * prv->bGeometry * bsdf.densityRev))
      * (prv->length <= 1.0f ? 0.0f : 1.0f);

    itr->B
      = (prv->B
        * Beta::beta(bsdf.densityRev)
        + bsdf.finite
        * vertex_merging
        * (prv->length <= _trim_light ? 0.0f : 1.0f)
        * Beta::beta(prv->a))
      * Beta::beta(edge.bGeometry * itr->a);

    bsdf = new_bsdf;

    prv = itr++;
  }

  if (bsdf.finite == 0) {
    --itr;
  }

  size = itr - path.data();
}

template <class Beta>
float UPGBase<Beta>::_vc_subweight_inv(const Connection& connection) {

  float Ap
    = (connection.light.A * Beta::beta(connection.light_bsdf.densityRev) + connection.light.finite * Beta::beta(connection.light.a))
    * Beta::beta(connection.edge.bGeometry * connection.eye_bsdf.densityRev);

  float Cp
    = (connection.eye.C * Beta::beta(connection.eye_bsdf.density) + connection.eye.finite * Beta::beta(connection.eye.c))
    * Beta::beta(connection.edge.fGeometry * connection.light_bsdf.density);

  return Ap + Cp + 1.0f;
}

template <class Beta>
float UPGBase<Beta>::_vm_subweight_inv(const Connection& connection) {
  float light_vertex_merging = 0.0f;
  float eye_vertex_merging = 0.0f;
  float connect_vertex_merging = 0.0f;

  if (_from_light) {
    light_vertex_merging += _clamp(Beta::beta(_circle / connection.light.a)); // light

    eye_vertex_merging += _clamp(Beta::beta(_circle * connection.eye.bGeometry * connection.eye_bsdf.density)) // light
      * (connection.eye.length <= 1.0f ? 0.0f : 1.0f);

    connect_vertex_merging += _clamp(Beta::beta(_circle * connection.edge.fGeometry * connection.light_bsdf.density))
      * (connection.eye.length * connection.light.length == 0.0f ? 0.0f : 1.0f); // light
  }
  else {
    light_vertex_merging += _clamp(Beta::beta(_circle * connection.light.bGeometry * connection.light_bsdf.densityRev)) // eye
      * (connection.light.length <= 1.0f ? 0.0f : 1.0f);

    eye_vertex_merging += _clamp(Beta::beta(_circle / connection.eye.c)); // eye

    connect_vertex_merging += _clamp(Beta::beta(_circle * connection.edge.bGeometry * connection.eye_bsdf.densityRev))
      * (connection.eye.length * connection.light.length == 0.0f ? 0.0f : 1.0f); // eye
  }

  float Bp
    = (connection.light.B * Beta::beta(connection.light_bsdf.densityRev) + connection.light_bsdf.finite
      * light_vertex_merging
      * Beta::beta(connection.light.a) * (connection.light.length <= _trim_light ? 0.0f : 1.0f))
    * Beta::beta(connection.edge.bGeometry * connection.eye_bsdf.densityRev);

  float Dp
    = (connection.eye.D * Beta::beta(connection.eye_bsdf.density) + connection.eye_bsdf.finite
      * eye_vertex_merging
      * Beta::beta(connection.eye.c) * (connection.eye.length <= _trim_eye ? 0.0f : 1.0f))
    * Beta::beta(connection.edge.fGeometry * connection.light_bsdf.density);

  return Beta::beta(_num_scattered) * (Bp + Dp + connect_vertex_merging);
}

template <class Beta>
float UPGBase<Beta>::_vm_biased_subweight_inv(const Connection& connection, float connect_vertex_merging) {
  float light_vertex_merging = 0.0f;
  float eye_vertex_merging = 0.0f;

  if (_from_light) {
    light_vertex_merging += _clamp(Beta::beta(_circle / connection.light.a)); // light

    eye_vertex_merging += _clamp(Beta::beta(_circle * connection.eye.bGeometry * connection.eye_bsdf.density)) // light
      * (connection.eye.length <= 1.0f ? 0.0f : 1.0f);
  }
  else {
    light_vertex_merging += _clamp(Beta::beta(_circle * connection.light.bGeometry * connection.light_bsdf.densityRev)) // eye
      * (connection.light.length <= 1.0f ? 0.0f : 1.0f);

    eye_vertex_merging += _clamp(Beta::beta(_circle / connection.eye.c)); // eye
  }

  float Bp
    = (connection.light.B * Beta::beta(connection.light_bsdf.densityRev) + connection.light_bsdf.finite
      * light_vertex_merging
      * Beta::beta(connection.light.a) * (connection.light.length <= _trim_light ? 0.0f : 1.0f))
    * Beta::beta(connection.edge.bGeometry * connection.eye_bsdf.densityRev);

  float Dp
    = (connection.eye.D * Beta::beta(connection.eye_bsdf.density) + connection.eye_bsdf.finite
      * eye_vertex_merging
      * Beta::beta(connection.eye.c) * (connection.eye.length <= _trim_eye ? 0.0f : 1.0f))
    * Beta::beta(connection.edge.fGeometry * connection.light_bsdf.density);

  return Beta::beta(_num_scattered) * (Bp + Dp + connect_vertex_merging);
}

template <class Beta>
float UPGBase<Beta>::_vc_weight(const Connection& connection) {
  if ((connection.eye.length + connection.light.length) < 2) {
    return 1.0f / _vc_subweight_inv(connection);
  }
  else {
    float vc = float(_enable_vc) * _vc_subweight_inv(connection);
    float vm = float(_enable_vm) * _vm_subweight_inv(connection);
    return 1.0f / (vc + vm);
  }
}

template <class Beta>
float UPGBase<Beta>::_vc_biased_weight(const Connection& connection, float vm_current) {
  if ((connection.eye.length + connection.light.length) < 2) {
    return 1.0f / _vc_subweight_inv(connection);
  }
  else {
    float vc = float(_enable_vc) * _vc_subweight_inv(connection);
    float vm = float(_enable_vm) * _vm_biased_subweight_inv(connection, vm_current);
    return 1.0f / (vc + vm);
  }
}

template <class Beta>
float UPGBase<Beta>::_vm_biased_weight(const Connection& connection, float vm_current) {
  float weight = _vc_biased_weight(connection, vm_current * (connection.eye.length == 0 ? 0.0f : 1.0f));

  return Beta::beta(_num_scattered) * vm_current * weight; // light
}

template <class Beta>
float UPGBase<Beta>::_weight_vm_eye(const Connection& connection) {
  float weight = _vc_weight(connection);

  return Beta::beta(float(_num_scattered)
    * _clamp(_circle * connection.edge.bGeometry * connection.eye_bsdf.densityRev)) * weight; // eye
}

template <class Beta>
float UPGBase<Beta>::_weight_vm_light(const Connection& connection) {
  float weight = _vc_weight(connection);

  return Beta::beta(float(_num_scattered)
    * _clamp(_circle * connection.edge.fGeometry * connection.light_bsdf.density)) * weight; // light
}

template <class Beta>
float UPGBase<Beta>::_density(
  random_generator_t& generator,
  const vec3& omega,
  const SurfacePoint& surface,
  const vec3& target) {
  return _scene->queryBSDF(surface).gathering_density(
    generator, _scene.get(),
    surface, { target, _radius }, omega);
}

template <class Beta>
vec3 UPGBase<Beta>::_connect(const Connection& connection) {
  return _scene->occluded(connection.eye.surface, connection.light.surface)
    * connection.light.throughput
    * connection.light_bsdf.throughput
    * connection.eye.throughput
    * connection.eye_bsdf.throughput
    * connection.edge.bCosTheta
    * connection.edge.fGeometry;
}

template <class Beta>
vec3 UPGBase<Beta>::_connect_light(const EyeVertex& eye, float Dp) {
  auto camera_bsdf = _scene->queryBSDF(eye.surface, vec3(0.0f), eye.omega);

  if (l1Norm(camera_bsdf.throughput) < FLT_MIN) {
    return vec3(0.0f);
  }

  float eye_vertex_merging = 0.f;

  if (_from_light) {
    eye_vertex_merging += _clamp(Beta::beta(_circle * eye.bGeometry * camera_bsdf.density)) // light
      * (eye.length <= 1.0f ? 0.0f : 1.0f);
  }
  else {
    eye_vertex_merging += _clamp(Beta::beta(_circle / eye.c));
  }

  auto lsdf = _scene->queryLSDF(eye.surface, eye.omega);

  float Cp
    = (eye.C * Beta::beta(camera_bsdf.density) + Beta::beta(eye.c) * eye.finite)
    * Beta::beta(lsdf.density);

  float weightInv = Cp + 1.0f + Beta::beta(float(_num_scattered)) * Dp;

  return lsdf.radiance
    * eye.throughput
    / weightInv;
}

template <class Beta>
vec3 UPGBase<Beta>::_connect_directional(const EyeVertex& eye, const LightSample& sample) {
  auto isect = _scene->intersect(eye.surface, -sample.normal());

  if (isect.material_id == sample.surface.material_id) {
    auto camera_bsdf = _scene->queryBSDF(eye.surface, -sample.normal(), eye.omega);

    float eye_vertex_merging = 0.f;

    if (_from_light) {
      eye_vertex_merging += _clamp(Beta::beta(_circle * eye.bGeometry * camera_bsdf.density)) // light
        * (eye.length <= 1.0f ? 0.0f : 1.0f);
    }
    else {
      eye_vertex_merging += _clamp(Beta::beta(_circle / eye.c));
    }

    float coeff = Beta::beta(abs(dot(sample.normal(), eye.surface.normal()))
      / distance2(isect.position(), eye.surface.position()));

    float Dp
      = (eye.D * Beta::beta(camera_bsdf.density) + camera_bsdf.finite
        * eye_vertex_merging
        * Beta::beta(eye.c) * (eye.length <= _trim_eye ? 0.0f : 1.0f))
      * coeff;

    float Cp
      = (eye.C * Beta::beta(camera_bsdf.density) + Beta::beta(eye.c) * eye.finite)
      * coeff;

    float weightInv = (Cp + 1.0f) * float(_enable_vc) + Beta::beta(_num_scattered) * Dp * float(_enable_vm);

    vec3 result = sample.radiance() / sample.light_density * _roulette_inv
      * eye.throughput
      * camera_bsdf.throughput
      * abs(dot(sample.normal(), eye.surface.normal()));

    return l1Norm(result) < FLT_EPSILON ? vec3(0.0f) : result / weightInv;
  }
  else {
    return vec3(0.0f);
  }
}

template <class Beta>
vec3 UPGBase<Beta>::_connect(const LightVertex& light, const EyeVertex& eye) {
  vec3 omega = normalize(eye.surface.position() - light.surface.position());

  Connection connection;
  connection.light = light;
  connection.light_bsdf = _scene->queryBSDF(light.surface, light.omega, omega);
  connection.eye = eye;
  connection.eye_bsdf = _scene->queryBSDF(eye.surface, -omega, eye.omega);
  connection.edge = Edge(light.surface, eye.surface, omega);

  auto throughput = _connect(connection);

  if (l1Norm(throughput) > FLT_EPSILON) {
    float vm_current = _clamp(Beta::beta(_circle * connection.edge.fGeometry * connection.light_bsdf.density))
      * (connection.eye.length == 0.0f ? 0.0f : 1.0f);

    float weight = _unbiased
      ? _vc_weight(connection)
      : _vc_biased_weight(connection, vm_current);

    return _connect(connection) * weight;
  }
  else {
    return vec3(0.0f);
  }
}

template <class Beta>
vec3 UPGBase<Beta>::_connect(
  render_context_t& context,
  const EyeVertex& eye) {
  size_t path = context.pixel_index;
  vec3 radiance = vec3(0.0f);

  if (eye.surface.is_camera()) {
    for (size_t index = _light_offsets[path], s = _light_offsets[path + 1]; index < s; ++index) {
      vec3 omega = normalize(_light_paths[index].surface.position() - eye.surface.position());

      radiance += _accumulate(
        context,
        omega,
        [&] {
        float camera_coefficient = _camera_coefficient(
          _light_paths[index].omega,
          _light_paths[index].surface.gnormal,
          _light_paths[index].surface.normal(),
          omega,
          eye.surface.normal());

        return _connect(_light_paths[index], eye) * context.focal_factor_y * camera_coefficient;
      });
    }
  }
  else {
    if (!_russian_roulette(*context.generator)) {
      LightSample sample = _scene->sampleLight(*context.generator);

      if (sample.kind == light_kind::area) {
        radiance += _connect(_sample_to_vertex(sample), eye);
      }
      else if (!eye.surface.is_camera()) {
        radiance += _connect_directional(eye, sample);
      }
    }

    for (size_t i = _light_offsets[path] + 1, s = _light_offsets[path + 1]; i < s; ++i) {
      radiance += _connect(_light_paths[i], eye);
    }
  }

  return radiance;
}

template <class Beta>
void UPGBase<Beta>::_scatter(random_generator_t& generator) {
  std::mutex mutex;
  std::condition_variable condition;
  std::atomic<size_t> counter(0);

  const size_t num_tasks = _threadpool.num_threads();
  const size_t num_photons = _num_photons / num_tasks;
  const size_t num_photons_last = _num_photons - num_photons * (num_tasks - 1);

  const size_t prev_paths_size = _light_paths.size();
  const size_t prev_offsets_size = _light_offsets.size();

  vector<vector<LightVertex>> paths(num_tasks - 1);
  vector<vector<uint32_t>> offsets(num_tasks - 1);

  for (size_t i = 0; i < num_tasks - 1; ++i) {
    paths.reserve(prev_paths_size / (num_tasks - 1));
  }

  for (size_t i = 0; i < num_tasks - 1; ++i) {
    _threadpool.exec([=, &generator, &paths, &offsets, &mutex, &condition, &counter] {
      auto local_generator = generator.clone();
      size_t size = 0;

      offsets[i].resize(1, 0);
      offsets[i].reserve(num_photons + 1);

      for (std::size_t j = 0; j < num_photons; ++j) {
        _traceLight(local_generator, paths[i], size);
        offsets[i].push_back(size);
      }

      paths[i].resize(size);

      if (counter.fetch_add(1) == num_tasks - 2) {
        std::unique_lock<std::mutex> lock(mutex);
        condition.notify_one();
      }
    });
  }

  size_t size = 0;

  _light_offsets.resize(1, 0);
  _light_offsets.reserve(prev_offsets_size);

  for (std::size_t i = 0; i < num_photons_last; ++i) {
    _traceLight(generator, _light_paths, size);
    _light_offsets.push_back(size);
  }

  _light_paths.resize(size);
  _light_paths.reserve(prev_paths_size);

  std::unique_lock<std::mutex> lock(mutex);
  condition.wait(lock, [&] { return counter == num_tasks - 1; });

  for (size_t i = 0; i < paths.size(); ++i) {
    _light_paths.insert(_light_paths.end(), paths[i].begin(), paths[i].end());

    uint32_t offset = _light_offsets.back();

    for (size_t j = 1; j < offsets[i].size(); ++j) {
      _light_offsets.push_back(offsets[i][j] + offset);
    }
  }

  _num_scattered = _num_photons;
  _num_scattered_inv = 1.0f / float(_num_scattered);
  _statistics.num_scattered += _num_scattered;

  time_scope_t _(_statistics.build_time);
  _vertices = v3::HashGrid3D<LightVertex>(&_light_paths, _radius);
}

template <class Beta>
vec3 UPGBase<Beta>::_gather(
  render_context_t& context,
  const EyeVertex& eye,
  const EyeVertex& tentative) {
  vec3 radiance = vec3(0.0f);

  _vertices.rQuery(
    [&](uint32_t index) {
      time_scope_t _(_statistics.merge_time);

      runtime_assert(!_light_paths[index].surface.is_light());

      if (_from_light && !_light_paths[index - 1].surface.is_light()) { // light
        radiance += _merge_light(*context.generator, _light_paths[index - 1], tentative) * _num_scattered_inv;
      }
      else if (!_from_light && !eye.surface.is_camera()) {
        radiance += _merge_eye(*context.generator, _light_paths[index], eye) * _num_scattered_inv;
      }
    },
    tentative.surface.position(),
    _radius);

  return radiance;
}

template <class Beta>
vec3 UPGBase<Beta>::_gather_biased(
  render_context_t& context,
  const EyeVertex& eye,
  const EyeVertex& tentative) {
  vec3 radiance = vec3(0.0f);

  _vertices.rQuery(
    [&](uint32_t index) {
      time_scope_t _(_statistics.merge_time);

      runtime_assert(!_light_paths[index].surface.is_light());

      if (_from_light && !_light_paths[index - 1].surface.is_light()) { // light
        radiance += _merge_biased(*context.generator, _light_paths[index - 1], _light_paths[index], tentative) * _num_scattered_inv;
      }
      else if (!_from_light && !eye.surface.is_camera()) {
        radiance += _merge_biased(*context.generator, _light_paths[index], _light_paths[index], eye) * _num_scattered_inv;
      }
    },
    tentative.surface.position(),
    _radius);

  return radiance;
}

template <class Beta>
vec3 UPGBase<Beta>::_merge_light(
  random_generator_t& generator,
  const LightVertex& light,
  const EyeVertex& eye) {
  vec3 omega = normalize(eye.surface.position() - light.surface.position());

  Connection connection;
  connection.light = light;
  connection.light_bsdf = _scene->queryBSDF(light.surface, light.omega, omega);
  connection.eye = eye;
  connection.eye_bsdf = _scene->queryBSDF(eye.surface, -omega, eye.omega);
  connection.edge = Edge(light.surface, eye.surface, omega);

  vec3 throughput = _connect(connection);

  if (l1Norm(throughput) < FLT_EPSILON) {
    return vec3(0.0f);
  }
  else {
    auto weight = _weight_vm_light(connection);
    time_scope_t _(_statistics.density_time);
    auto density = _unbiased
      ? _density(generator, connection.light.omega,
        connection.light.surface, connection.eye.surface.position())
      : 1.0f / (_circle * connection.edge.fGeometry * connection.light_bsdf.density);

    return throughput * density * weight;
  }
}

template <class Beta>
vec3 UPGBase<Beta>::_merge_eye(
  random_generator_t& generator,
  const LightVertex& light,
  const EyeVertex& eye) {
  vec3 omega = normalize(eye.surface.position() - light.surface.position());

  Connection connection;
  connection.light = light;
  connection.light_bsdf = _scene->queryBSDF(light.surface, light.omega, omega);
  connection.eye = eye;
  connection.eye_bsdf = _scene->queryBSDF(eye.surface, -omega, eye.omega);
  connection.edge = Edge(light.surface, eye.surface, omega);

  vec3 throughput = _connect(connection);

  if (l1Norm(throughput) < FLT_EPSILON) {
    return vec3(0.0f);
  }
  else {
    auto weight = _weight_vm_eye(connection);
    time_scope_t _(_statistics.density_time);
    auto density = _unbiased
      ? _density(generator, connection.eye.omega,
        connection.eye.surface, connection.light.surface.position())
      : 1.0f / (_circle * connection.edge.bGeometry * connection.eye_bsdf.densityRev);

    return throughput * density * weight;
  }
}

template <class Beta>
vec3 UPGBase<Beta>::_merge_biased(
  random_generator_t& generator,
  const LightVertex& light,
  const LightVertex& tentative,
  const EyeVertex& eye) {
  vec3 omega = normalize(eye.surface.position() - light.surface.position());

  Connection connection;
  connection.light = light;
  connection.light_bsdf = _scene->queryBSDF(light.surface, light.omega, omega);
  connection.eye = eye;
  connection.eye_bsdf = _scene->queryBSDF(eye.surface, -omega, eye.omega);
  connection.edge = Edge(light.surface, eye.surface, omega);

  vec3 throughput = tentative.throughput
    * connection.eye.throughput
    * connection.eye_bsdf.throughput
    * _roulette;

  if (l1Norm(throughput) < FLT_EPSILON) {
    return vec3(0.0f);
  }
  else {
    auto weight = _vm_biased_weight(connection,
      //Beta::beta(_circle * connection.edge.fGeometry * connection.light_bsdf.density));
      Beta::beta(_circle / tentative.a));

    time_scope_t _(_statistics.density_time);
    auto density = 1.0f / _circle;

    return throughput * density * weight;
  }
}

template <class Beta>
float UPGBase<Beta>::_clamp(float x) const {
  return min(_clamp_const, x);
}

template <class Beta>
bool UPGBase<Beta>::_russian_roulette(random_generator_t& generator) const {
  return _roulette < generator.sample();
}

UPGb::UPGb(
  const shared<const Scene>& scene,
  bool unbiased,
  bool enable_vc,
  bool enable_vm,
  bool from_light,
  float lights,
  float roulette,
  size_t numPhotons,
  float radius,
  float alpha,
  float beta,
  size_t num_threads)
  : UPGBase<VariableBeta>(
    scene,
    unbiased,
    enable_vc,
    enable_vm,
    from_light,
    lights,
    roulette,
    numPhotons,
    radius,
    alpha,
    beta,
    num_threads) {
  VariableBeta::init(beta);
}

template class UPGBase<FixedBeta<0>>;
template class UPGBase<FixedBeta<1>>;
template class UPGBase<FixedBeta<2>>;
template class UPGBase<VariableBeta>;

}
