#include <UPG.hpp>
#include <Edge.hpp>

namespace haste {

template <class Beta, GatherMode Mode>
UPGBase<Beta, Mode>::UPGBase(
    const shared<const Scene>& scene,
    size_t minSubpath,
    bool enable_vc,
    bool enable_vm,
    float lights,
    float roulette,
    size_t numPhotons,
    float radius,
    float beta,
    size_t num_threads)
    : Technique(scene, num_threads)
    , _num_photons(numPhotons)
    , _num_scattered(0)
    , _minSubpath(minSubpath)
    , _enable_vc(enable_vc)
    , _enable_vm(enable_vm)
    , _lights(lights)
    , _roulette(roulette)
    , _radius(radius)
    , _circle(pi<float>() * radius * radius)
{ }

template <class Beta, GatherMode Mode>
string UPGBase<Beta, Mode>::name() const {
    return Mode == GatherMode::Unbiased ? "Unbiased Photon Gathering" : "Vertex Connection and Merging";
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_traceEye(render_context_t& context, Ray ray) {
    light_path_t light_path;
    vec3 radiance = vec3(0.0f);

    if (_russian_roulette(*context.generator)) {
        return radiance;
    }

    if (_enable_vc) {
        _traceLight(*context.generator, light_path);
    }

    EyeVertex eye[2];
    size_t itr = 0, prv = 1;

    eye[prv].surface = _camera_surface(context);
    eye[prv].omega = -ray.direction;
    eye[prv].throughput = vec3(1.0f) / _roulette;
    eye[prv].specular = 0.0f;
    eye[prv].c = 0;
    eye[prv].C = 0;
    eye[prv].d = 0;
    eye[prv].D = 0;

    if (_enable_vc) {
        radiance += _connect_eye(context, eye[prv], light_path);
    }

    SurfacePoint surface = _scene->intersect(eye[prv].surface, ray.direction);

    while (surface.is_light()) {
        if (_enable_vc) {
            radiance += eye[prv].throughput * _lights * _scene->queryRadiance(surface, -ray.direction);
        }

        surface = _scene->intersect(surface, ray.direction);
    }

    if (!surface.is_present()) {
        return radiance;
    }

    if (_russian_roulette(*context.generator)) {
        return radiance;
    }

    eye[itr].surface = surface;
    eye[itr].omega = -ray.direction;

    auto edge = Edge(eye[prv], eye[itr]);

    eye[itr].throughput = eye[prv].throughput / _roulette;
    eye[itr].specular = 0.0f;
    eye[itr].c = 1.0f / Beta::beta(edge.fGeometry);
    eye[itr].C = 0.0f;
    eye[itr].d = 0.0f;
    eye[itr].D = 0.0f;

    std::swap(itr, prv);

    while (true) {
        if (_enable_vm) {
            radiance += _gather(*context.generator, eye[prv]);
        }

        if (_enable_vc) {
            radiance += _connect(eye[prv], light_path);
        }

        auto bsdf = _scene->sampleBSDF(*context.generator, eye[prv].surface, eye[prv].omega);

        while (true) {
            surface = _scene->intersect(surface, bsdf.omega);

            if (!surface.is_present()) {
                return radiance;
            }

            eye[itr].surface = surface;
            eye[itr].omega = -bsdf.omega;

            auto edge = Edge(eye[prv], eye[itr]);

            eye[itr].throughput
                = eye[prv].throughput
                * bsdf.throughput
                * edge.bCosTheta;

            if (l1Norm(eye[itr].throughput) < FLT_EPSILON) {
              return radiance;
            }

            eye[itr].throughput /= bsdf.density;

            eye[prv].specular = max(eye[prv].specular, bsdf.specular);
            eye[itr].specular = bsdf.specular;
            eye[itr].c = 1.0f / Beta::beta(edge.fGeometry * bsdf.density);

            eye[itr].C
                = (eye[prv].C
                    * Beta::beta(bsdf.densityRev)
                    + (1.0f - eye[prv].specular) * eye[prv].c)
                * Beta::beta(edge.bGeometry)
                * eye[itr].c;

            eye[itr].d = eye[itr].c;

            eye[itr].D
                = (eye[prv].D
                    * Beta::beta(bsdf.densityRev)
                    + (1.0f - bsdf.specular)
                    * min(1.0f, Beta::beta(_circle) / eye[prv].d)
                    * eye[prv].d)
                * Beta::beta(edge.bGeometry)
                * eye[itr].d;

            if (surface.is_light()) {
                if (_enable_vc) {
                    radiance += _connect_light(eye[itr]);
                }
            }
            else {
                break;
            }
        }

        std::swap(itr, prv);

        if (_russian_roulette(*context.generator)) {
            return radiance;
        }

        eye[prv].throughput /= _roulette;
    }

    return radiance;
}

template <class Beta, GatherMode Mode>
void UPGBase<Beta, Mode>::_preprocess(random_generator_t& generator) {
    _scatter(generator);
}

template <class Beta, GatherMode Mode> template <bool First, class Appender>
void UPGBase<Beta, Mode>::_traceLight(random_generator_t& generator, Appender& path) {
    size_t begin = path.size();
    size_t itr = path.size() + 1, prv = path.size();

    LightSample light = _scene->sampleLight(generator);

    path.emplace_back();
    path[prv].surface = light.surface;
    path[prv].omega = vec3(0.0f);
    path[prv].throughput = light.radiance() / light.areaDensity();
    path[prv].specular = 0.0f;
    path[prv].a = 1.0f / Beta::beta(light.areaDensity());
    path[prv].A = 0.0f;
    path[prv].b = 0.0f;
    path[prv].B = 0.0f;

    while (!_russian_roulette(generator)) {
        auto bsdf = _scene->sampleBSDF(generator, path[prv].surface, path[prv].omega);

        auto surface = _scene->intersectMesh(path[prv].surface, bsdf.omega);

        if (!surface.is_present()) {
            break;
        }

        path.emplace_back();

        path[itr].surface = surface;
        path[itr].omega = -bsdf.omega;

        auto edge = Edge(path[prv], path[itr]);

        path[itr].throughput
            = path[prv].throughput
            * bsdf.throughput
            * edge.bCosTheta
            / _roulette;

        if (l1Norm(path[itr].throughput) < FLT_EPSILON) {
            path.pop_back();
            break;
        }

        path[itr].throughput /= bsdf.density;

        path[prv].specular = max(path[prv].specular, bsdf.specular);
        path[itr].specular = bsdf.specular;
        path[itr].a = 1.0f / Beta::beta(edge.fGeometry * bsdf.density);

        path[itr].A
            = (path[prv].A
                * Beta::beta(bsdf.densityRev)
                + (1.0f - path[prv].specular) * path[prv].a)
            * Beta::beta(edge.bGeometry)
            * path[itr].a;

        path[itr].b = itr - begin < 2 ? 0.0f : path[itr].a;

        path[itr].B
            = (path[prv].B
                * Beta::beta(bsdf.densityRev)
                + (1.0f - bsdf.specular)
                * min(1.0f, Beta::beta(_circle * path[prv].bGeometry * bsdf.densityRev))
                * path[prv].b)
            * Beta::beta(edge.bGeometry)
            * path[itr].b;

        path[itr].bGeometry = edge.bGeometry;

        if (bsdf.specular == 1.0f) {
            path[prv] = path[itr];
            path.pop_back();
        }
        else {
            prv = itr;
            ++itr;
        }
    }

    auto bsdf = _scene->sampleBSDF(
        generator,
        path[prv].surface,
        path[prv].omega);

    if (bsdf.specular == 1.0f) {
        path.pop_back();
    }

    if (!First) {
        path[begin] = path[path.size() - 1];
        path.pop_back();
    }
}

template <class Beta, GatherMode Mode>
void UPGBase<Beta, Mode>::_traceLight(
    random_generator_t& generator,
    vector<LightVertex>& path) {
    _traceLight<false, vector<LightVertex>>(generator, path);
}

template <class Beta, GatherMode Mode>
void UPGBase<Beta, Mode>::_traceLight(
    random_generator_t& generator,
    light_path_t& path) {
    _traceLight<true, light_path_t>(generator, path);
}

template <class Beta, GatherMode Mode> template <bool SkipDirectVM>
float UPGBase<Beta, Mode>::_weightVC(
    const LightVertex& light,
    const BSDFQuery& lightBSDF,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    const Edge& edge) {

    float skip_direct_vm = SkipDirectVM ? 0.0f : 1.0f;

    float Ap
        = (light.A * Beta::beta(lightBSDF.densityRev) + (1.0f - light.specular) * light.a)
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev);

    float Bp
        = (light.B * Beta::beta(lightBSDF.densityRev) + (1.0f - lightBSDF.specular) * min(1.0f, Beta::beta(_circle * light.bGeometry * lightBSDF.densityRev)) * light.b)
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev);

    float Cp
        = (eye.C * Beta::beta(eyeBSDF.density) + (1.0f - eye.specular) * eye.c)
        * Beta::beta(edge.fGeometry * lightBSDF.density);

    float Dp
        = (eye.D * Beta::beta(eyeBSDF.density) + (1.0f - eyeBSDF.specular) * min(1.0f, Beta::beta(_circle) / eye.d) * eye.d)
        * Beta::beta(edge.fGeometry * lightBSDF.density);

    float weightInv
        = Ap + Beta::beta(_num_scattered) * Bp + Cp + Beta::beta(_num_scattered) * Dp
        + Beta::beta(float(_num_scattered) * min(1.0f, _circle * edge.bGeometry * eyeBSDF.densityRev)) * skip_direct_vm + 1.0f;

    return 1.0f / weightInv;
}

template <class Beta, GatherMode Mode>
float UPGBase<Beta, Mode>::_weightVM(
    const LightVertex& light,
    const BSDFQuery& lightBSDF,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    const Edge& edge) {
    float weight = _weightVC<false>(light, lightBSDF, eye, eyeBSDF, edge);

    return Beta::beta(float(_num_scattered) * min(1.0f, _circle * edge.bGeometry * eyeBSDF.densityRev)) * weight;
}

template <class Beta, GatherMode Mode>
float UPGBase<Beta, Mode>::_density(
    random_generator_t& generator,
    const LightVertex& light,
    const EyeVertex& eye,
    const BSDFQuery& eyeQuery,
    const Edge& edge) {

    if (Mode == GatherMode::Unbiased) {
        return _scene->queryBSDF(eye.surface).gathering_density(
            generator,
            _scene.get(),
            eye.surface,
            { light.surface.position(), _radius },
            eye.omega);
    }
    else {
        return 1.0f / (edge.bGeometry * eyeQuery.densityRev * _circle);
    }
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_connect_light(const EyeVertex& eye) {
    if (!eye.surface.is_light()) {
        return vec3(0.0f);
    }

    auto lsdf = _scene->queryLSDF(eye.surface, eye.omega);

    float Cp
        = (eye.C * Beta::beta(lsdf.omegaDensity()) + eye.c * (1.0f - eye.specular))
        * Beta::beta(lsdf.areaDensity());

    float Dp = eye.D / eye.c * Beta::beta(lsdf.omegaDensity());

    float weightInv = Cp + Beta::beta(float(_num_scattered)) * Dp + 1.0f;

    return lsdf.radiance()
        * eye.throughput
        / weightInv;
}


template <class Beta, GatherMode Mode> template <bool SkipDirectVM>
vec3 UPGBase<Beta, Mode>::_connect(const LightVertex& light, const EyeVertex& eye) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    auto edge = Edge(light, eye, omega);

    auto weight = _weightVC<SkipDirectVM>(light, lightBSDF, eye, eyeBSDF, edge);

    vec3 radiance = _combine(
        _scene->occluded(eye.surface, light.surface)
            * light.throughput
            * lightBSDF.throughput
            * eye.throughput
            * eyeBSDF.throughput
            * edge.bCosTheta
            * edge.fGeometry,
        weight);

    return radiance;
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_connect(
    const EyeVertex& eye,
    const light_path_t& path) {
    vec3 radiance = vec3(0.0f);

    radiance += _connect<true>(path[0], eye);

    for (size_t i = 1; i < path.size(); ++i) {
        radiance += _connect<false>(path[i], eye);
    }

    return radiance;
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_connect_eye(
    render_context_t& context,
    const EyeVertex& eye,
    const light_path_t& path) {
    vec3 radiance = vec3(0.0f);

    for (size_t i = 1; i < path.size(); ++i) {
        vec3 omega = normalize(path[i].surface.position() - eye.surface.position());

        radiance += _accumulate(
            context,
            omega,
            [&] {
                float correct_normal = abs(dot(omega, path[i].surface.gnormal)
                    / dot(omega, path[i].surface.normal()));

                return _connect<true>(path[i], eye) * context.focal_factor_y * correct_normal;
            });
    }

    return radiance;
}

template <class Beta, GatherMode Mode>
void UPGBase<Beta, Mode>::_scatter(random_generator_t& generator) {
    _num_scattered = 0;

    std::atomic<size_t> total_num_scattered(0);

    vector<LightVertex> vertices = generate<LightVertex>(_threadpool, _num_photons,
        [this, &total_num_scattered, &generator](size_t num_photons) {
        auto local_generator = generator.clone();

        size_t num_scattered = 0;

        vector<LightVertex> vertices;

        while (vertices.size() < num_photons) {
            _traceLight(local_generator, vertices);
            ++num_scattered;
        }

        total_num_scattered += num_scattered;

        return vertices;
    });

    _num_scattered = total_num_scattered;

    _vertices = v3::HashGrid3D<LightVertex>(move(vertices), _radius);
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_gather(random_generator_t& generator, const EyeVertex& eye) {
    auto eyeBSDF = _scene->sampleBSDF(generator, eye.surface, eye.omega);
    SurfacePoint surface = _scene->intersectMesh(eye.surface, eyeBSDF.omega);

    if (!surface.is_present()) {
        return vec3(0.0f);
    }

    vec3 radiance = vec3(0.0f);

    _vertices.rQuery(
        [&](const LightVertex& light) {
            if (Mode == GatherMode::Unbiased) {
                radiance += _merge(generator, light, eye);
            }
            else {
                BSDFQuery query;
                query.throughput = eyeBSDF.throughput;
                query.density = eyeBSDF.densityRev;
                query.densityRev = eyeBSDF.density;
                radiance += _merge(generator, light, eye, query);
            }
        },
        surface.position(),
        _radius);

    return radiance / float(_num_scattered);
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_merge(
    random_generator_t& generator,
    const LightVertex& light,
    const EyeVertex& eye) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    auto edge = Edge(light, eye, omega);

    vec3 result = _scene->occluded(eye.surface, light.surface)
        * light.throughput
        * lightBSDF.throughput
        * eye.throughput
        * eyeBSDF.throughput
        * edge.bCosTheta
        * edge.fGeometry;

    if (l1Norm(result) < FLT_EPSILON) {
        return vec3(0.0f);
    }
    else {
        auto density = _density(generator, light, eye, eyeBSDF, edge);
        auto weight = _weightVM(light, lightBSDF, eye, eyeBSDF, edge);

        return _combine(result * density, weight);
    }
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_merge(
    random_generator_t& generator,
    const LightVertex& light,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);

    auto edge = Edge(light, eye, omega);

    auto weight = _weightVM(light, lightBSDF, eye, eyeBSDF, edge);
    auto density = 1.0f / (eyeBSDF.densityRev * pi<float>() * _radius * _radius);

    vec3 result = _scene->occluded(light.surface, eye.surface)
        * light.throughput
        * lightBSDF.throughput
        * eye.throughput
        * eyeBSDF.throughput
        * edge.fCosTheta;

    return _combine(std::isfinite(density) ? result * density : vec3(0.0f), weight);
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_combine(vec3 throughput, float weight) {
    return l1Norm(throughput) < FLT_EPSILON ? vec3(0.0f) : throughput * weight;
}

template <class Beta, GatherMode Mode>
bool UPGBase<Beta, Mode>::_russian_roulette(random_generator_t& generator) const {
    return _roulette < generator.sample();
}

UPGb::UPGb(
    const shared<const Scene>& scene,
    size_t minSubpath,
    bool enable_vc,
    bool enable_vm,
    float lights,
    float roulette,
    size_t numPhotons,
    float radius,
    float beta,
    size_t num_threads)
    : UPGBase<VariableBeta, GatherMode::Unbiased>(
        scene,
        minSubpath,
        enable_vc,
        enable_vm,
        lights,
        roulette,
        numPhotons,
        radius,
        beta,
        num_threads) {
    VariableBeta::init(beta);
}

template class UPGBase<FixedBeta<0>, GatherMode::Unbiased>;
template class UPGBase<FixedBeta<1>, GatherMode::Unbiased>;
template class UPGBase<FixedBeta<2>, GatherMode::Unbiased>;
template class UPGBase<VariableBeta, GatherMode::Unbiased>;

VCMb::VCMb(
    const shared<const Scene>& scene,
    size_t minSubpath,
    bool enable_vc,
    bool enable_vm,
    float lights,
    float roulette,
    size_t numPhotons,
    float radius,
    float beta,
    size_t num_threads)
    : UPGBase<VariableBeta, GatherMode::Biased>(
        scene,
        minSubpath,
        enable_vc,
        enable_vm,
        lights,
        roulette,
        numPhotons,
        radius,
        beta,
        num_threads) {
    VariableBeta::init(beta);
}

template class UPGBase<FixedBeta<0>, GatherMode::Biased>;
template class UPGBase<FixedBeta<1>, GatherMode::Biased>;
template class UPGBase<FixedBeta<2>, GatherMode::Biased>;
template class UPGBase<VariableBeta, GatherMode::Biased>;

}
