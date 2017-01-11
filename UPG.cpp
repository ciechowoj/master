#include <UPG.hpp>
#include <Edge.hpp>
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
    , _initial_radius(radius)
    , _alpha(alpha)
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
    prv->throughput = vec3(1.0f) / _roulette;
    prv->specular = 0.0f;
    prv->c = 0;
    prv->C = 0;
    prv->d = 0;
    prv->D = 0;

    float d = 0.0f;

    while (true) {
        if (_enable_vc) {
            if (prv->surface.is_camera()) {
                radiance += _connect_eye(context, *prv);
            }
            else {
                radiance += _connect(context, *prv);
            }
        }

        auto bsdf = _scene->sampleBSDF(*context.generator, prv->surface, prv->omega);

        while (true) {
            surface = _scene->intersect(surface, bsdf.omega);

            if (!surface.is_present()) {
                return radiance;
            }

            itr->surface = surface;
            itr->omega = -bsdf.omega;

            auto edge = Edge(*prv, *itr);

            itr->throughput
                = prv->throughput
                * bsdf.throughput
                * edge.bCosTheta;

            if (l1Norm(itr->throughput) < FLT_EPSILON) {
              return radiance;
            }

            itr->throughput /= bsdf.density;

            prv->specular = max(prv->specular, bsdf.specular);
            itr->specular = bsdf.specular;
            itr->c = 1.0f / Beta::beta(edge.fGeometry * bsdf.density);

            itr->C
                = (prv->C
                    * Beta::beta(bsdf.densityRev)
                    + (1.0f - prv->specular) * prv->c)
                * Beta::beta(edge.bGeometry)
                * itr->c;

            itr->d = itr->c * d;

            itr->D
                = (prv->D
                    * Beta::beta(bsdf.densityRev)
                    + (1.0f - bsdf.specular)
                    // * min(1.0f, Beta::beta(_circle) / prv->d)
                    * min(1.0f, Beta::beta(_circle * prv->bGeometry * bsdf.densityRev))
                    * prv->d)
                * Beta::beta(edge.bGeometry)
                * itr->d;

            itr->bGeometry = edge.bGeometry;

            if (surface.is_light()) {
                if (_enable_vc) {
                    radiance += _connect_light(*itr);
                }
            }
            else {
                break;
            }
        }

        itr->throughput /= _roulette;

        if (_enable_vm) {
            time_scope_t _2(_metadata.gather_time);

            if (prv->surface.is_camera()) {
                radiance += _gather(*context.generator, *prv, bsdf, *itr);
            }
            else {
                radiance += _gather(*context.generator, *prv, bsdf, *itr);
            }
        }

        d = 1.0f;

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
typename UPGBase<Beta>::LightVertex UPGBase<Beta>::_sample_light(random_generator_t& generator) {
    LightSample light = _scene->sampleLight(generator);

    LightVertex vertex;
    vertex.surface = light.surface;
    vertex.omega = vertex.surface.normal();
    vertex.throughput = light.radiance() / light.areaDensity() / _roulette;
    vertex.specular = 0.0f;
    vertex.a = 1.0f / Beta::beta(light.areaDensity());
    vertex.A = 0.0f;
    vertex.b = 0.0f;
    vertex.B = 0.0f;

    return vertex;
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

    while (!_russian_roulette(generator)) {
        auto bsdf = _scene->sampleBSDF(generator, prv->surface, prv->omega);

        auto surface = _scene->intersectMesh(prv->surface, bsdf.omega);

        if (!surface.is_present()) {
            break;
        }

        itr->surface = surface;
        itr->omega = -bsdf.omega;

        auto edge = Edge(*prv, *itr);

        itr->throughput
            = prv->throughput
            * bsdf.throughput
            * edge.bCosTheta
            / _roulette;

        if (l1Norm(itr->throughput) < FLT_EPSILON) {
            break;
        }

        itr->throughput /= bsdf.density;

        prv->specular = max(prv->specular, bsdf.specular);
        itr->specular = bsdf.specular;

        itr->a = 1.0f / Beta::beta(edge.fGeometry * bsdf.density);

        itr->A
            = (prv->A
                * Beta::beta(bsdf.densityRev)
                + (1.0f - prv->specular) * prv->a)
            * Beta::beta(edge.bGeometry)
            * itr->a;

        itr->b = itr - begin < 1 ? 0.0f : itr->a;

        itr->B
            = (prv->B
                * Beta::beta(bsdf.densityRev)
                + (1.0f - bsdf.specular)
                // * min(1.0f, Beta::beta(_circle * prv->bGeometry * bsdf.densityRev))
                * min(1.0f, Beta::beta(_circle) / prv->b)
                * prv->b)
            * Beta::beta(edge.bGeometry)
            * itr->b;

        itr->bGeometry = edge.bGeometry;

        if (bsdf.specular != 1.0f) {
            prv = itr;
            ++itr;
        }
    }

    auto bsdf = _scene->sampleBSDF(generator, prv->surface, prv->omega);

    if (bsdf.specular == 1.0f) {
        --itr;
    }

    size = itr - path.data();
}

template <class Beta> template <bool SkipDirectVM>
float UPGBase<Beta>::_weightVC(
    const LightVertex& light,
    const BSDFQuery& lightBSDF,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    const Edge& edge) {

    float skip_direct_vm = SkipDirectVM ? 0.0f : 1.0f;

    float Ap = 0.0f;
        /* = (light.A * Beta::beta(lightBSDF.densityRev) + (1.0f - light.specular) * light.a)
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev);*/

    float Bp
        = (light.B * Beta::beta(lightBSDF.densityRev) + (1.0f - lightBSDF.specular)
            // * min(1.0f, Beta::beta(_circle * light.bGeometry * lightBSDF.densityRev)) * light.b)
            * min(1.0f, Beta::beta(_circle) / light.b) * light.b)
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev);

    float Cp = 0.0f;
        /* = (eye.C * Beta::beta(eyeBSDF.density) + (1.0f - eye.specular) * eye.c)
        * Beta::beta(edge.fGeometry * lightBSDF.density); */

    float Dp
        = (eye.D * Beta::beta(eyeBSDF.density) + (1.0f - eyeBSDF.specular)
            // * min(1.0f, Beta::beta(_circle) / eye.d) * eye.d)
            * min(1.0f, Beta::beta(_circle * eye.bGeometry * eyeBSDF.density)) * eye.d)
        * Beta::beta(edge.fGeometry * lightBSDF.density);

    float weightInv
        = Ap + Beta::beta(_num_scattered) * Bp + Cp + Beta::beta(_num_scattered) * Dp
        + Beta::beta(float(_num_scattered)
        // * min(1.0f, _circle * edge.bGeometry * eyeBSDF.densityRev))
        * min(1.0f, _circle * edge.fGeometry * lightBSDF.density))
        * skip_direct_vm; // + 1.0f;

    return 1.0f / weightInv;
}

template <class Beta>
float UPGBase<Beta>::_weightVM(
    const LightVertex& light,
    const BSDFQuery& lightBSDF,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    const Edge& edge) {
    float weight = _weightVC<false>(light, lightBSDF, eye, eyeBSDF, edge);

    // return Beta::beta(float(_num_scattered) * min(1.0f, _circle * edge.bGeometry * eyeBSDF.densityRev)) * weight;
    return Beta::beta(float(_num_scattered) * min(1.0f, _circle * edge.fGeometry * lightBSDF.density)) * weight;
}

template <class Beta>
float UPGBase<Beta>::_density(
    random_generator_t& generator,
        const vec3& omega,
        const SurfacePoint& surface,
        const BSDFQuery& bsdf,
        const vec3& target,
        const Edge& edge) {
    if (_unbiased) {
        return _scene->queryBSDF(surface).gathering_density(
            generator, _scene.get(),
            surface, { target, _radius }, omega);
    }
    else {
        // return 1.0f / (edge.bGeometry * bsdf.densityRev * _circle);
        return 1.0f / (_circle * edge.fGeometry * bsdf.density);
    }
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
    if (!eye.surface.is_light()) {
        return vec3(0.0f);
    }

    auto lsdf = _scene->queryLSDF(eye.surface, eye.omega);
    auto bsdf = _scene->queryBSDF(eye.surface, vec3(0.0f), eye.omega);

    float Cp
        = (eye.C * Beta::beta(bsdf.density) + eye.c * (1.0f - eye.specular))
        * Beta::beta(lsdf.density);

    float Dp = eye.D / eye.c * Beta::beta(bsdf.density);

    float weightInv = Cp + Beta::beta(float(_num_scattered)) * Dp + 1.0f;

    return lsdf.radiance
        * eye.throughput
        / weightInv;
}


template <class Beta> template <bool SkipDirectVM>
vec3 UPGBase<Beta>::_connect(const LightVertex& light, const EyeVertex& eye) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    auto edge = Edge(light, eye, omega);

    auto weight = _weightVC<SkipDirectVM>(light, lightBSDF, eye, eyeBSDF, edge);

    return _combine(_connect(light, lightBSDF, eye, eyeBSDF, edge), weight);
}

template <class Beta>
vec3 UPGBase<Beta>::_connect(
    render_context_t& context,
    const EyeVertex& eye) {
    size_t path = context.pixel_index;
    vec3 radiance = vec3(0.0f);

    if (!_russian_roulette(*context.generator)) {
        radiance += _connect<true>(_sample_light(*context.generator), eye);
    }

    for (size_t i = _light_offsets[path] + 1, s = _light_offsets[path + 1]; i < s; ++i) {
        radiance += _connect<false>(_light_paths[i], eye);
    }

    return radiance;
}

template <class Beta>
vec3 UPGBase<Beta>::_connect_eye(
    render_context_t& context,
    const EyeVertex& eye) {
    size_t path = context.pixel_index;
    vec3 radiance = vec3(0.0f);

    for (size_t i = _light_offsets[path], s = _light_offsets[path + 1]; i < s; ++i) {
        vec3 omega = normalize(_light_paths[i].surface.position() - eye.surface.position());

        radiance += _accumulate(
            context,
            omega,
            [&] {
                float correct_normal = abs(
                    (dot(omega, _light_paths[i].surface.gnormal)) *
                    dot(_light_paths[i].omega, _light_paths[i].surface.normal()) /
                    (dot(omega, _light_paths[i].surface.normal()) *
                    dot(_light_paths[i].omega, _light_paths[i].surface.gnormal)));

                vec3 camera = eye.surface.toSurface(omega);
                float correct_cos_inv = 1.0f / pow(abs(camera.y), 3.0f);

                return _connect<true>(_light_paths[i], eye) * context.focal_factor_y * correct_normal * correct_cos_inv;
            });
    }

    return radiance;
}

/*template <class Beta>
vec3 UPGBase<Beta>::_gather_eye(
    render_context_t& context,
    const EyeVertex& eye) {
    SurfacePoint surface;

    {
        time_scope_t _(_metadata.intersect_time);
        surface = _scene->intersectMesh(eye.surface, -eye.omega);

        if (!surface.is_present()) {
            return vec3(0.0f);
        }
    }

    vec3 radiance = vec3(0.0f);

    _vertices.rQuery(
        [&](uint32_t index) {
            const LightVertex& light = _light_paths[index];

            time_scope_t _(_metadata.merge_time);

            vec3 omega = normalize(light.surface.position() - eye.surface.position());

            radiance += _accumulate(
            context,
            omega,
            [&] {
                float correct_normal = abs(dot(omega, light.surface.gnormal)
                    / dot(omega, light.surface.normal()));

                return _merge(*context.generator, light, eye)
                    * _num_scattered_inv * correct_normal;
            });
        },
        surface.position(),
        _radius);

    return radiance;
}*/

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
    random_generator_t& generator,
    const EyeVertex& eye,
    const BSDFQuery& eye_bsdf,
    const EyeVertex& tentative) {
    vec3 radiance = vec3(0.0f);

    _vertices.rQuery(
        [&](uint32_t index) {
            time_scope_t _(_metadata.merge_time);
            radiance += _merge(generator,
                _light_paths[index - 1], _light_paths[index], eye, tentative) * _num_scattered_inv;
        },
        tentative.surface.position(),
        _radius);

    return radiance;
}

template <class Beta>
vec3 UPGBase<Beta>::_merge(
    random_generator_t& generator,
    const LightVertex& light0,
    const LightVertex& light1,
    const EyeVertex& eye0,
    const EyeVertex& eye1) {

    // return _merge_eye(generator, light1, eye0);
    return _merge_light(generator, light0, eye1);
}

template <class Beta>
vec3 UPGBase<Beta>::_merge_light(
    random_generator_t& generator,
    const LightVertex& light,
    const EyeVertex& eye) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto light_bsdf = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eye_bsdf = _scene->queryBSDF(eye.surface, -omega, eye.omega);
    auto edge = Edge(light, eye, omega);

    vec3 throughput = _connect(light, light_bsdf, eye, eye_bsdf, edge);

    if (l1Norm(throughput) < FLT_EPSILON) {
        return vec3(0.0f);
    }
    else {
        auto weight = _weightVM(light, light_bsdf, eye, eye_bsdf, edge);
        time_scope_t _(_metadata.density_time);
        auto density = _density(generator, light.omega,
            light.surface, light_bsdf, eye.surface.position(), edge);

        return _combine(throughput * density, weight);
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
    auto edge = Edge(light, eye, omega);

    vec3 throughput = _connect(light, light_bsdf, eye, eye_bsdf, edge);

    if (l1Norm(throughput) < FLT_EPSILON) {
        return vec3(0.0f);
    }
    else {
        auto weight = _weightVM(light, light_bsdf, eye, eye_bsdf, edge);
        time_scope_t _(_metadata.density_time);
        auto density = _density(generator, eye.omega,
            eye.surface, eye_bsdf, light.surface.position(), edge);

        return _combine(throughput * density, weight);
    }
}

template <class Beta>
vec3 UPGBase<Beta>::_combine(vec3 throughput, float weight) {
    return l1Norm(throughput) < FLT_EPSILON ? vec3(0.0f) : throughput * weight;
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
