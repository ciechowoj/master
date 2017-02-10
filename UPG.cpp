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
    _metadata.num_photons = _num_photons;
    _metadata.roulette = _roulette;
    _metadata.radius = _radius;
    _metadata.alpha = _alpha;
    _metadata.beta = beta;
}

template <class Beta>
string UPGBase<Beta>::name() const {
    return _unbiased ? "Unbiased Photon Gathering" : "Vertex Connection and Merging";
}

template <class Beta>
string UPGBase<Beta>::id() const {
    return string(_unbiased ? "UPG" : "VCM")
        + "_r" + std::to_string(_initial_radius)
        + "_a" + std::to_string(_metadata.alpha)
        + "_b" + std::to_string(_metadata.beta);
}

template <class Beta>
vec3 UPGBase<Beta>::_traceEye(render_context_t& context, Ray ray) {
    time_scope_t _0(_metadata.trace_eye_time);

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
    prv->specular = 0.0f;
    prv->c = 0;
    prv->C = 0;
    prv->D = 0;
    prv->length = 0;
    prv->pGlossiness = USHRT_MAX;
    prv->ppGlossiness = USHRT_MAX;

    BSDFSample bsdf = _scene->sampleBSDF(*context.generator, prv->surface, prv->omega);
    BSDFSample new_bsdf;

    while (true) {
        if (_enable_vc) {
            radiance += _connect(context, *prv);
        }

        while (true) {
            surface = _scene->intersect(surface, bsdf.omega);

            if (!surface.is_present()) {
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
            itr->pGlossiness = bsdf.glossiness;
            itr->ppGlossiness = prv->pGlossiness;
            itr->length = prv->length + 1.0f;

            prv->specular = max(prv->specular, bsdf.specular);
            itr->specular = bsdf.specular;
            itr->c = 1.0f / Beta::beta(edge.fGeometry * bsdf.density);

            itr->C
                = (prv->C
                    * Beta::beta(bsdf.densityRev)
                    + (1.0f - prv->specular) * prv->c)
                * Beta::beta(edge.bGeometry)
                * itr->c;

            float vertex_merging = 0.0f;

            if (_merge_from_light && _merge_from_eye) {
                if (itr->pGlossiness <= prv->ppGlossiness) {
                    vertex_merging += _clamp(Beta::beta(_circle * prv->bGeometry * bsdf.densityRev))
                        * (prv->length <= 1.0f ? 0.0f : 1.0f);
                }

                if (prv->pGlossiness < new_bsdf.glossiness) {
                    vertex_merging += _clamp(Beta::beta(_circle) / prv->c); // eye
                }
            }
            else if (_merge_from_light) {
                vertex_merging = _clamp(Beta::beta(_circle * prv->bGeometry * bsdf.densityRev));
            }
            else if (_merge_from_eye) {
                vertex_merging = _clamp(Beta::beta(_circle) / prv->c);
            }

            itr->D
                = (prv->D
                    * Beta::beta(bsdf.densityRev)
                    + (1.0f - bsdf.specular)
                    * vertex_merging
                    * (prv->length <= _trim_eye ? 0.0f : 1.0f)
                    * prv->c)
                * Beta::beta(edge.bGeometry)
                * itr->c;

            if (surface.is_light()) {
                if (_enable_vc) {
                    radiance += _connect_light(*itr);
                }
            }
            else {
                bsdf = new_bsdf;
                break;
            }
        }

        itr->throughput *= _roulette_inv;

        if (_enable_vm) {
            time_scope_t _2(_metadata.gather_time);
            radiance += _gather(context, *prv, bsdf, *itr);
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
    time_scope_t _0(_metadata.scatter_time);

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
    vertex.specular = 0.0f;
    vertex.a = sample.kind == light_kind::directional ? 0.0f : 1.0f / Beta::beta(sample.combined_density());
    vertex.A = 0.0f;
    vertex.B = 0.0f;
    vertex.length = 0.0f;
    vertex.pGlossiness = 0.0f;
    vertex.ppGlossiness = 0.0f;

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
        itr->ppGlossiness = prv->pGlossiness;
        itr->pGlossiness = bsdf.glossiness;
        itr->length = prv->length + 1.0f;

        prv->specular = max(prv->specular, bsdf.specular);
        itr->specular = bsdf.specular;

        itr->a = 1.0f / Beta::beta(edge.fGeometry * bsdf.density);

        itr->A
            = (prv->A
                * Beta::beta(bsdf.densityRev)
                + (1.0f - prv->specular) * prv->a)
            * Beta::beta(edge.bGeometry)
            * itr->a;

        float vertex_merging = 0.0f;

        if (_merge_from_light && _merge_from_eye) {
            if (itr->pGlossiness < prv->ppGlossiness) {
                vertex_merging += _clamp(Beta::beta(_circle * prv->bGeometry * bsdf.densityRev))
                    * (prv->length <= 1.0f ? 0.0f : 1.0f);
            }

            if (prv->pGlossiness <= new_bsdf.glossiness) {
                vertex_merging += _clamp(Beta::beta(_circle) / prv->a); // light
            }
        }
        else if (_merge_from_light) {
            vertex_merging = _clamp(Beta::beta(_circle) / prv->a);
        }
        else if (_merge_from_eye) {
            vertex_merging = _clamp(Beta::beta(_circle * prv->bGeometry * bsdf.densityRev));
        }

        itr->B
            = (prv->B
                * Beta::beta(bsdf.densityRev)
                + (1.0f - bsdf.specular)
                * vertex_merging
                * (prv->length <= _trim_light ? 0.0f : 1.0f)
                * prv->a)
            * Beta::beta(edge.bGeometry)
            * itr->a;

        bsdf = new_bsdf;

        if (bsdf.specular != 1.0f) {
            prv = itr;
            ++itr;
        }
        else {
            *prv = *itr;
        }
    }

    if (bsdf.specular == 1.0f) {
        --itr;
    }

    size = itr - path.data();
}

template <class Beta>
float UPGBase<Beta>::_weightVC(
    const LightVertex& light,
    const BSDFQuery& lightBSDF,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    const Edge& edge) {

    float Ap
        = (light.A * Beta::beta(lightBSDF.densityRev) + (1.0f - light.specular) * light.a)
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev);

    float Cp
        = (eye.C * Beta::beta(eyeBSDF.density) + (1.0f - eye.specular) * eye.c)
        * Beta::beta(edge.fGeometry * lightBSDF.density);

    float light_vertex_merging = 0.0f;
    float eye_vertex_merging = 0.0f;
    float connect_vertex_merging = 0.0f;

    if ((eye.length + light.length) >= 2.0f) {
        if (_merge_from_light && _merge_from_eye) {
            if (lightBSDF.glossiness < light.ppGlossiness) {
                light_vertex_merging += _clamp(Beta::beta(_circle * light.bGeometry * lightBSDF.densityRev)) // eye
                    * (light.length <= 1.0f ? 0.0f : 1.0f);
            }

            if (eyeBSDF.glossiness <= eye.ppGlossiness) {
                eye_vertex_merging += _clamp(Beta::beta(_circle * eye.bGeometry * eyeBSDF.density)) // light
                    * (eye.length <= 1.0f ? 0.0f : 1.0f);
            }

            if (light.pGlossiness <= eyeBSDF.glossiness) {
                light_vertex_merging += _clamp(Beta::beta(_circle) / light.a); // light
            }
            else {
                connect_vertex_merging += _clamp(Beta::beta(_circle * edge.bGeometry * eyeBSDF.densityRev))
                    * (light.length == 0.0f ? 0.0f : 1.0f); // eye
            }

            if (eye.pGlossiness < lightBSDF.glossiness) {
                eye_vertex_merging += _clamp(Beta::beta(_circle) / eye.c); // eye
            }
            else {
                connect_vertex_merging += _clamp(Beta::beta(_circle * edge.fGeometry * lightBSDF.density))
                    * (eye.length == 0.0f ? 0.0f : 1.0f); // light
            }
        }
        else if (_merge_from_light) {
            light_vertex_merging = _clamp(Beta::beta(_circle) / light.a);
            eye_vertex_merging = _clamp(Beta::beta(_circle * eye.bGeometry * eyeBSDF.density));
            connect_vertex_merging = _clamp(Beta::beta(_circle * edge.fGeometry * lightBSDF.density));
        }
        else if (_merge_from_eye) {
            light_vertex_merging = _clamp(Beta::beta(_circle * light.bGeometry * lightBSDF.densityRev));
            eye_vertex_merging = _clamp(Beta::beta(_circle) / eye.c);
            connect_vertex_merging = _clamp(Beta::beta(_circle * edge.bGeometry * eyeBSDF.densityRev));
        }
    }

    float Bp
        = (light.B * Beta::beta(lightBSDF.densityRev) + (1.0f - lightBSDF.specular)
            * light_vertex_merging
            * light.a * (light.length <= _trim_light ? 0.0f : 1.0f))
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev);

    float Dp
        = (eye.D * Beta::beta(eyeBSDF.density) + (1.0f - eyeBSDF.specular)
            * eye_vertex_merging
            * eye.c * (eye.length <= _trim_eye ? 0.0f : 1.0f))
        * Beta::beta(edge.fGeometry * lightBSDF.density);

    float weightInv
        = Ap + Beta::beta(_num_scattered) * Bp + Cp + Beta::beta(_num_scattered) * Dp
        + Beta::beta(float(_num_scattered))
        * connect_vertex_merging + 1.0f;

    return 1.0f / weightInv;
}

template <class Beta>
float UPGBase<Beta>::_weight_vm_eye(
    const LightVertex& light,
    const BSDFQuery& lightBSDF,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    const Edge& edge) {
    float weight = _weightVC(light, lightBSDF, eye, eyeBSDF, edge);

    return Beta::beta(float(_num_scattered)
        * _clamp(_circle * edge.bGeometry * eyeBSDF.densityRev)) * weight; // eye
}

template <class Beta>
float UPGBase<Beta>::_weight_vm_light(
    const LightVertex& light,
    const BSDFQuery& lightBSDF,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    const Edge& edge) {
    float weight = _weightVC(light, lightBSDF, eye, eyeBSDF, edge);

    return Beta::beta(float(_num_scattered)
        * _clamp(_circle * edge.fGeometry * lightBSDF.density)) * weight; // light
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
vec3 UPGBase<Beta>::_connect(
    const LightVertex& light, const BSDFQuery& light_bsdf,
    const EyeVertex& eye, const BSDFQuery& eye_bsdf, const Edge& edge) {
    return _scene->occluded(eye.surface, light.surface)
        * light.throughput
        * light_bsdf.throughput
        * eye.throughput
        * eye_bsdf.throughput
        * edge.bCosTheta
        * edge.fGeometry;
}

template <class Beta>
vec3 UPGBase<Beta>::_connect_light(const EyeVertex& eye) {
    auto bsdf = _scene->queryBSDF(eye.surface, vec3(0.0f), eye.omega);

    if (l1Norm(bsdf.throughput) < FLT_MIN) {
        return vec3(0.0f);
    }

    auto lsdf = _scene->queryLSDF(eye.surface, eye.omega);

    float Cp
        = (eye.C * Beta::beta(bsdf.density) + eye.c * (1.0f - eye.specular))
        * Beta::beta(lsdf.density);

    // float Dp = eye.D / eye.c * Beta::beta(bsdf.density); // eye

    float Dp
        = (eye.D * Beta::beta(bsdf.density) + (1.0f - bsdf.specular)
            // * _clamp(Beta::beta(_circle) / eye.d) * eye.d) // eye
            * _clamp(Beta::beta(_circle * eye.bGeometry * bsdf.density)) * eye.c) // light
        * Beta::beta(lsdf.density);

    float weightInv = Cp + Beta::beta(float(_num_scattered)) * Dp * (eye.length <= 2.0f ? 0.0f : 1.0f) + 1.0f;

    return lsdf.radiance
        * eye.throughput
        / weightInv;
}

template <class Beta>
vec3 UPGBase<Beta>::_connect_directional(const EyeVertex& eye, const LightSample& sample) {
    auto isect = _scene->intersect(eye.surface, -sample.normal());

    if (isect.material_id == sample.surface.material_id) {
        auto eyeBSDF = _scene->queryBSDF(eye.surface, -sample.normal(), eye.omega);

        float eye_vertex_merging = 0.0f;

        if (eye.length >= 2.0f && _merge_from_light && _merge_from_eye) {
            if (eyeBSDF.glossiness <= eye.ppGlossiness) {
                eye_vertex_merging += _clamp(Beta::beta(_circle * eye.bGeometry * eyeBSDF.density)) // light
                    * (eye.length <= 1.0f ? 0.0f : 1.0f);
            }

            if (eye.pGlossiness < USHRT_MAX) {
                eye_vertex_merging += _clamp(Beta::beta(_circle) / eye.c); // eye
            }
        }

        float coeff = Beta::beta(abs(dot(sample.normal(), eye.surface.normal()))
                / distance2(isect.position(), eye.surface.position()));

        float Dp
            = (eye.D * Beta::beta(eyeBSDF.density) + (1.0f - eyeBSDF.specular)
            * eye_vertex_merging
            * eye.c * (eye.length <= _trim_eye ? 0.0f : 1.0f))
            * coeff;

        float Cp
            = (eye.C * Beta::beta(eyeBSDF.density) + eye.c * (1.0f - eye.specular))
            * coeff;

        float weightInv = Cp + Beta::beta(_num_scattered) * Dp + 1.0f;

        vec3 result = sample.radiance() / sample.light_density * _roulette_inv
            * eye.throughput
            * eyeBSDF.throughput
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

    auto light_bsdf = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eye_bsdf = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    auto edge = Edge(light.surface, eye.surface, omega);

    auto weight = _weightVC(light, light_bsdf, eye, eye_bsdf, edge);

    return _connect(light, light_bsdf, eye, eye_bsdf, edge) * weight;
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
    _metadata.num_scattered += _num_scattered;

    time_scope_t _(_metadata.build_time);
    _vertices = v3::HashGrid3D<LightVertex>(&_light_paths, _radius);
}

template <class Beta>
vec3 UPGBase<Beta>::_gather(
    render_context_t& context,
    const EyeVertex& eye,
    const BSDFQuery& eye_bsdf,
    const EyeVertex& tentative) {
    vec3 radiance = vec3(0.0f);

    _vertices.rQuery(
        [&](uint32_t index) {
            time_scope_t _(_metadata.merge_time);

            runtime_assert(!_light_paths[index].surface.is_light());

            if (!eye.surface.is_camera() || !_light_paths[index - 1].surface.is_light()) {
                if (_light_paths[index].pGlossiness <= tentative.pGlossiness) { // light
                    radiance += _merge_light(*context.generator, _light_paths[index - 1], tentative) * _num_scattered_inv;
                }
                else if (eye.surface.is_camera()) { // eye
                    vec3 omega = normalize(_light_paths[index].surface.position() - eye.surface.position());

                radiance += _accumulate(context, omega, [&] {
                        float normal_coefficient = _normal_coefficient(
                            _light_paths[index].omega,
                            _light_paths[index].surface.gnormal,
                            _light_paths[index].surface.normal(),
                            omega);

                        return _merge_eye(*context.generator, _light_paths[index], eye)
                            * normal_coefficient * _num_scattered_inv;
                    });
                }
                else {
                    radiance += _merge_eye(*context.generator, _light_paths[index], eye) * _num_scattered_inv;
                }
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

    auto light_bsdf = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eye_bsdf = _scene->queryBSDF(eye.surface, -omega, eye.omega);
    auto edge = Edge(light.surface, eye.surface, omega);

    vec3 throughput = _connect(light, light_bsdf, eye, eye_bsdf, edge);

    if (l1Norm(throughput) < FLT_EPSILON) {
        return vec3(0.0f);
    }
    else {
        auto weight = _weight_vm_light(light, light_bsdf, eye, eye_bsdf, edge);
        time_scope_t _(_metadata.density_time);
        auto density = _unbiased
            ? _density(generator, light.omega,
                light.surface, eye.surface.position())
            : 1.0f / (_circle * edge.fGeometry * light_bsdf.density);

        return throughput * density * weight;
    }
}

template <class Beta>
vec3 UPGBase<Beta>::_merge_eye(
    random_generator_t& generator,
    const LightVertex& light,
    const EyeVertex& eye) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto light_bsdf = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eye_bsdf = _scene->queryBSDF(eye.surface, -omega, eye.omega);
    auto edge = Edge(light.surface, eye.surface, omega);

    vec3 throughput = _connect(light, light_bsdf, eye, eye_bsdf, edge);

    if (l1Norm(throughput) < FLT_EPSILON) {
        return vec3(0.0f);
    }
    else {
        auto weight = _weight_vm_eye(light, light_bsdf, eye, eye_bsdf, edge);
        time_scope_t _(_metadata.density_time);
        auto density = _unbiased
            ? _density(generator, eye.omega,
                eye.surface, light.surface.position())
            : 1.0f / (_circle * edge.bGeometry * eye_bsdf.densityRev);

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
