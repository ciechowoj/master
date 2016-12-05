#include <UPG.hpp>
#include <Edge.hpp>

namespace haste {

template <class Beta, GatherMode Mode>
UPGBase<Beta, Mode>::UPGBase(
    size_t minSubpath,
    float roulette,
    size_t numPhotons,
    size_t numGather,
    float radius,
    size_t num_threads)
    : Technique(num_threads)
    , _numPhotons(numPhotons)
    , _numGather(numGather)
    , _minSubpath(minSubpath)
    , _roulette(roulette)
    , _radius(radius)
{ }

template <class Beta, GatherMode Mode>
void UPGBase<Beta, Mode>::preprocess(
    const shared<const Scene>& scene,
    RandomEngine& engine,
    const function<void(string, float)>& progress,
    bool parallel)
{
    Technique::preprocess(scene, engine, progress, parallel);

    _scatter(engine);
}

template <class Beta, GatherMode Mode>
string UPGBase<Beta, Mode>::name() const {
    return Mode == GatherMode::Unbiased ? "Unbiased Photon Gathering" : "Vertex Connection and Merging";
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_traceEye(render_context_t& context, Ray ray) {
    float radius = _radius;
    light_path_t light_path;

    _traceLight(*context.engine, light_path);

    vec3 radiance = vec3(0.0f);
    EyeVertex eye[2];
    size_t itr = 0, prv = 1;

    eye[prv].surface = _camera_surface(context);
    eye[prv].omega = -ray.direction;
    eye[prv].throughput = vec3(1.0f);
    eye[prv].specular = 0.0f;
    eye[prv].c = 0;
    eye[prv].C = 0;
    eye[prv].d = 0;
    eye[prv].D = 0;

    radiance += _connect_eye(context, eye[prv], light_path, radius);

    RayIsect isect = _scene->intersect(ray.origin, ray.direction);

    while (isect.isLight()) {
        radiance += _scene->queryRadiance(isect, -ray.direction);
        isect = _scene->intersect(isect.position(), ray.direction);
    }

    if (!isect.isPresent()) {
        return radiance;
    }

    eye[itr].surface = _scene->querySurface(isect);
    eye[itr].omega = -ray.direction;

    auto edge = Edge(eye[prv], eye[itr]);

    eye[itr].throughput = vec3(1.0f);
    eye[itr].specular = 0.0f;
    eye[itr].c = 1.0f / Beta::beta(edge.fGeometry);
    eye[itr].C = 0;
    eye[itr].d = 0.0f;
    eye[itr].D = 0;

    std::swap(itr, prv);

    size_t path_size = 2;

    while (true) {
        radiance += _gather(*context.engine, eye[prv], radius);
        radiance += _connect(eye[prv], light_path, radius);

        auto bsdf = _scene->sampleBSDF(*context.engine, eye[prv].surface, eye[prv].omega);

        while (true) {
            isect = _scene->intersect(isect.position(), bsdf.omega);

            if (!isect.isPresent()) {
                return radiance;
            }

            eye[itr].surface = _scene->querySurface(isect);
            eye[itr].omega = -bsdf.omega;

            auto edge = Edge(eye[prv], eye[itr]);

            eye[itr].throughput
                = eye[prv].throughput
                * bsdf.throughput
                * edge.bCosTheta
                / bsdf.density;

            eye[prv].specular = max(eye[prv].specular, bsdf.specular);
            eye[itr].specular = bsdf.specular;
            eye[itr].c = 1.0f / Beta::beta(edge.fGeometry * bsdf.density);

            eye[itr].C
                = (eye[prv].C
                    * Beta::beta(bsdf.densityRev)
                    + eye[prv].c * (1.0f - eye[prv].specular))
                * Beta::beta(edge.bGeometry)
                * eye[itr].c;

            eye[itr].d = 1.0f;

            eye[itr].D
                = (eye[prv].D
                    * Beta::beta(bsdf.densityRev)
                    + eye[prv].d * (1.0f - bsdf.specular))
                * Beta::beta(edge.bGeometry)
                * eye[itr].c;

            if (isect.isLight()) {
                radiance += _connect_light(eye[itr], radius);
            }
            else {
                break;
            }
        }

        std::swap(itr, prv);

        float roulette = path_size < _minSubpath ? 1.0f : _roulette;
        float uniform = sampleUniform1(*context.engine).value();

        if (roulette < uniform) {
            return radiance;
        }
        else {
            eye[prv].throughput /= roulette;
            ++path_size;
        }
    }

    return radiance;
}

template <class Beta, GatherMode Mode> template <bool First, class Appender>
void UPGBase<Beta, Mode>::_traceLight(RandomEngine& engine, Appender& path) {
    size_t itr = path.size(), prv = path.size();

    LightSample light = _scene->sampleLight(engine);

    if (First) {
        path.emplace_back();
        path[itr].surface = light.surface;
        path[itr].omega = vec3(0.0f);
        path[itr].throughput = light.radiance() / light.areaDensity();
        path[itr].specular = 0.0f;
        path[itr].a = 1.0f / Beta::beta(light.areaDensity());
        path[itr].A = 0.0f;
        path[itr].B = 0.0f;
        ++itr;
    }

    RayIsect isect = _scene->intersectMesh(light.position(), light.omega());

    if (!isect.isPresent()) {
        return;
    }

    path.emplace_back();
    path[itr].surface = _scene->querySurface(isect);
    path[itr].omega = -light.omega();

    auto edge = Edge(light, path[itr]);

    path[itr].throughput = light.radiance() * edge.bCosTheta / light.density();
    path[itr].specular = 0.0f;
    path[itr].a = 1.0f / Beta::beta(edge.fGeometry * light.omegaDensity());
    path[itr].A = Beta::beta(edge.bGeometry) * path[itr].a / Beta::beta(light.areaDensity());
    path[itr].B = 0.0f;

    prv = itr;
    ++itr;

    size_t path_size = 2;
    float roulette = path_size < _minSubpath ? 1.0f : _roulette;
    float uniform = sampleUniform1(engine).value();

    while (uniform < roulette) {
        auto bsdf = _scene->sampleBSDF(engine, path[prv].surface, path[prv].omega);

        isect = _scene->intersectMesh(path[prv].surface.position(), bsdf.omega);

        if (!isect.isPresent()) {
            break;
        }

        ++path_size;
        path.emplace_back();

        path[itr].surface = _scene->querySurface(isect);
        path[itr].omega = -bsdf.omega;

        edge = Edge(path[prv], path[itr]);

        path[itr].throughput
            = path[prv].throughput
            * bsdf.throughput
            * edge.bCosTheta
            / (bsdf.density * roulette);

        path[prv].specular = max(path[prv].specular, bsdf.specular);
        path[itr].specular = bsdf.specular;

        path[itr].a = 1.0f / Beta::beta(edge.fGeometry * bsdf.density);

        path[itr].A
            = (path[prv].A
                * Beta::beta(bsdf.densityRev)
                + path[prv].a * (1.0f - path[prv].specular))
            * Beta::beta(edge.bGeometry)
            * path[itr].a;

        path[itr].B
            = (path[prv].B
                * Beta::beta(bsdf.densityRev)
                + (1.0f - bsdf.specular))
            * Beta::beta(edge.bGeometry)
            * path[itr].a;

        if (bsdf.specular == 1.0f) {
            path[prv] = path[itr];
            path.pop_back();
        }
        else {
            prv = itr;
            ++itr;
        }

        roulette = path_size < _minSubpath ? 1.0f : _roulette;
        uniform = sampleUniform1(engine).value();
    }

    auto bsdf = _scene->sampleBSDF(
        engine,
        path[prv].surface,
        path[prv].omega);

    if (bsdf.specular == 1.0f) {
        path.pop_back();
    }
}

template <class Beta, GatherMode Mode>
void UPGBase<Beta, Mode>::_traceLight(
    RandomEngine& engine,
    vector<LightVertex>& path) {
    _traceLight<false, vector<LightVertex>>(engine, path);
}

template <class Beta, GatherMode Mode>
void UPGBase<Beta, Mode>::_traceLight(
    RandomEngine& engine,
    light_path_t& path) {
    _traceLight<true, light_path_t>(engine, path);
}

template <class Beta, GatherMode Mode> template <bool SkipDirectVM>
float UPGBase<Beta, Mode>::_weightVC(
    const LightVertex& light,
    const BSDFQuery& lightBSDF,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    const Edge& edge,
    float radius) {
    float eta = float(_numPhotons) * pi<float>() * radius * radius;

    float skip_direct_vm = SkipDirectVM ? 0.0f : 1.0f;

    float Ap
        = (light.A * Beta::beta(lightBSDF.densityRev) + light.a * (1.0f - light.specular))
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev);

    float Bp
        = light.B * Beta::beta(lightBSDF.densityRev)
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev);

    float Cp
        = (eye.C * Beta::beta(eyeBSDF.density) + eye.c * (1.0f - eye.specular))
        * Beta::beta(edge.fGeometry * lightBSDF.density);

    float Dp
        = (eye.D * Beta::beta(eyeBSDF.density) + eye.d * (1.0f - eyeBSDF.specular))
        * Beta::beta(edge.fGeometry * lightBSDF.density);

    float weightInv
        = Ap + eta * Bp + Cp + eta * Dp
        + Beta::beta(eta * edge.bGeometry * eyeBSDF.densityRev * skip_direct_vm) + 1.0f;

    return 1.0f / weightInv;
}

template <class Beta, GatherMode Mode>
float UPGBase<Beta, Mode>::_weightVM(
    const LightVertex& light,
    const BSDFQuery& lightBSDF,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    const Edge& edge,
    float radius) {
    float eta = float(_numPhotons) * pi<float>() * radius * radius;

    float weight = _weightVC<false>(light, lightBSDF, eye, eyeBSDF, edge, radius);

    return Beta::beta(eta * edge.bGeometry * eyeBSDF.densityRev) * weight;
}

template <class Beta, GatherMode Mode>
float UPGBase<Beta, Mode>::_density(
    RandomEngine& engine,
    const LightVertex& light,
    const EyeVertex& eye,
    const BSDFQuery& eyeQuery,
    const Edge& edge,
    float radius) {

    if (Mode == GatherMode::Unbiased) {
        float L = 16777216.0f;
        float N = 1.0f;

        const auto& eyeBSDF = _scene->queryBSDF(eye.surface);

        auto omega = eye.surface.toSurface(eye.omega);

        bounding_sphere_t target;
        target.center = eye.surface.toSurface(light.surface.position() - eye.surface.position());
        target.radius = radius;

        while (N < L) {
            auto sample = eyeBSDF.sample_bounded(engine, target, omega);
            sample.omega = eye.surface.toWorld(sample.omega);

            RayIsect isect = _scene->intersectMesh(
                eye.surface.position(),
                sample.omega);

            if (isect.isPresent()) {
                float distance_sq = distance2(light.surface.position(), isect.position());

                if (distance_sq < radius * radius) {
                    return N / sample.adjust;
                }
            }

            N += 1.0f;
        }

        return INFINITY;
    }
    else {
        return 1.0f / (edge.bGeometry * eyeQuery.densityRev * pi<float>() * radius * radius);
    }
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_connect_light(const EyeVertex& eye, float radius) {
    if (!eye.surface.is_light()) {
        return vec3(0.0f);
    }

    float eta = float(_numPhotons) * pi<float>() * radius * radius;

    auto lsdf = _scene->queryLSDF(eye.surface, eye.omega);

    float Cp
        = (eye.C * Beta::beta(lsdf.omegaDensity()) + eye.c * (1.0f - eye.specular))
        * Beta::beta(lsdf.areaDensity());

    float Dp = eye.D / eye.c * Beta::beta(lsdf.omegaDensity());

    float weightInv = Cp + eta * Dp + 1.0f;

    return lsdf.radiance()
        * eye.throughput
        / weightInv;
}


template <class Beta, GatherMode Mode> template <bool SkipDirectVM>
vec3 UPGBase<Beta, Mode>::_connect(const LightVertex& light, const EyeVertex& eye, float radius) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    auto edge = Edge(light, eye, omega);

    auto weight = _weightVC<SkipDirectVM>(light, lightBSDF, eye, eyeBSDF, edge, radius);

    vec3 radiance = _combine(
        _scene->occluded(eye.surface.position(), light.surface.position())
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
    const light_path_t& path,
    float radius) {
    vec3 radiance = vec3(0.0f);

    radiance += _connect<true>(path[0], eye, radius);

    for (size_t i = 1; i < path.size(); ++i) {
        radiance += _connect<false>(path[i], eye, radius);
    }

    return radiance;
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_connect_eye(
    render_context_t& context,
    const EyeVertex& eye,
    const light_path_t& path,
    float radius) {
    vec3 radiance = vec3(0.0f);

    for (size_t i = 1; i < path.size(); ++i) {
        vec3 omega = path[i].surface.position() - eye.surface.position();

        radiance += _accumulate(
            context,
            omega,
            [&] {
                float correct_normal =
                    abs(dot(path[i].omega, path[i].surface.normal())
                    / dot(path[i].omega, path[i].surface.gnormal));

                return _connect<true>(path[i], eye, radius) * context.focal_factor_y * correct_normal;
            });
    }

    return radiance;
}

template <class Beta, GatherMode Mode>
void UPGBase<Beta, Mode>::_scatter(RandomEngine& engine) {
    vector<LightVertex> vertices;

    for (size_t i = 0; i < _numPhotons; ++i) {
        _traceLight(engine, vertices);
    }

    _vertices = v3::HashGrid3D<LightVertex>(move(vertices), _radius);
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_gather(RandomEngine& engine, const EyeVertex& eye, float& radius) {
    auto eyeBSDF = _scene->sampleBSDF(engine, eye.surface, eye.omega);
    RayIsect isect = _scene->intersectMesh(eye.surface.position(), eyeBSDF.omega);

    if (!isect.isPresent()) {
        return vec3(0.0f);
    }

    vec3 radiance = vec3(0.0f);

    radius = _radius;

    _vertices.rQuery(
        [&](const LightVertex& light) {
            if (Mode == GatherMode::Unbiased) {
                radiance += _merge(engine, light, eye, radius);
            }
            else {
                BSDFQuery query;
                query.throughput = eyeBSDF.throughput;
                query.density = eyeBSDF.densityRev;
                query.densityRev = eyeBSDF.density;
                radiance += _merge(engine, light, eye, query, radius);
            }
        },
        isect.position(),
        radius);

    return radiance / float(_numPhotons);
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_merge(
    RandomEngine& engine,
    const LightVertex& light,
    const EyeVertex& eye,
    float radius) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    auto edge = Edge(light, eye, omega);

    auto weight = _weightVM(light, lightBSDF, eye, eyeBSDF, edge, radius);
    auto density = _density(engine, light, eye, eyeBSDF, edge, radius);

    vec3 result = _scene->occluded(eye.surface.position(), light.surface.position())
        * light.throughput
        * lightBSDF.throughput
        * eye.throughput
        * eyeBSDF.throughput
        * edge.bCosTheta
        * edge.fGeometry;

    return _combine(std::isfinite(density) ? result * density : vec3(0.0f), weight);
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_merge(
    RandomEngine& engine,
    const LightVertex& light,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    float radius) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);

    auto edge = Edge(light, eye, omega);

    auto weight = _weightVM(light, lightBSDF, eye, eyeBSDF, edge, radius);
    auto density = _density(engine, light, eye, eyeBSDF, edge, radius);

    vec3 result = _scene->occluded(eye.surface.position(), light.surface.position())
        * light.throughput
        * lightBSDF.throughput
        * eye.throughput
        * eyeBSDF.throughput
        * edge.bCosTheta
        * edge.fGeometry;

    return _combine(std::isfinite(density) ? result * density : vec3(0.0f), weight);
}

template <class Beta, GatherMode Mode>
vec3 UPGBase<Beta, Mode>::_combine(vec3 throughput, float weight) {
    return throughput * weight;
}

UPGb::UPGb(
    size_t minSubpath,
    float roulette,
    size_t numPhotons,
    size_t numGather,
    float radius,
    float beta,
    size_t num_threads)
    : UPGBase<VariableBeta, GatherMode::Unbiased>(
        minSubpath,
        roulette,
        numPhotons,
        radius,
        numGather,
        num_threads) {
    VariableBeta::init(beta);
}

template class UPGBase<FixedBeta<0>, GatherMode::Unbiased>;
template class UPGBase<FixedBeta<1>, GatherMode::Unbiased>;
template class UPGBase<FixedBeta<2>, GatherMode::Unbiased>;
template class UPGBase<VariableBeta, GatherMode::Unbiased>;

VCMb::VCMb(
    size_t minSubpath,
    float roulette,
    size_t numPhotons,
    size_t numGather,
    float radius,
    float beta,
    size_t num_threads)
    : UPGBase<VariableBeta, GatherMode::Biased>(
        minSubpath,
        roulette,
        numPhotons,
        radius,
        numGather,
        num_threads) {
    VariableBeta::init(beta);
}

template class UPGBase<FixedBeta<0>, GatherMode::Biased>;
template class UPGBase<FixedBeta<1>, GatherMode::Biased>;
template class UPGBase<FixedBeta<2>, GatherMode::Biased>;
template class UPGBase<VariableBeta, GatherMode::Biased>;

}
