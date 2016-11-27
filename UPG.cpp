#include <iostream>
#include <UPG.hpp>
#include <Edge.hpp>

namespace haste {

template <class Beta>
UPGBase<Beta>::UPGBase(
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

template <class Beta>
void UPGBase<Beta>::preprocess(
    const shared<const Scene>& scene,
    RandomEngine& engine,
    const function<void(string, float)>& progress,
    bool parallel)
{
    Technique::preprocess(scene, engine, progress, parallel);

    _scatter(engine);
}

template <class Beta>
string UPGBase<Beta>::name() const {
    return "Unbiased Photon Gathering";
}

template <class Beta>
vec3 UPGBase<Beta>::_traceEye(
    render_context_t& context,
    Ray ray) {
    vec3 radiance = vec3(0.0f);

    RayIsect isect = _scene->intersect(ray.origin, ray.direction);

    while (isect.isLight()) {
        radiance += _scene->queryRadiance(isect, -ray.direction);
        isect = _scene->intersect(isect.position(), ray.direction);
    }

    if (!isect.isPresent()) {
        return radiance;
    }

    fixed_vector<LightVertex, _maxSubpath> light_path;
    _traceLight(*context.engine, light_path);

    EyeVertex eye[2];
    size_t itr = 0, prv = 1;
    float radius = _radius;

    /*eye[itr].surface = _scene->querySurface(isect);
    eye[itr].omega = vec3(0.0f, 0.0f, -1.0f);
    eye[itr].throughput = eye.direction;
    eye[itr].specular = 1.0f;
    eye[itr].c = 0;
    eye[itr].C = 0;
    eye[itr].d = 0;
    eye[itr].D = 0;

    radiance += _gather(*context.engine, eye[itr], radius);
    // radiance += _connect(*context.engine, eye[itr], light_path, radius);
    std::swap(itr, prv); */

    eye[itr].surface = _scene->querySurface(isect);
    eye[itr].omega = -ray.direction;
    eye[itr].throughput = vec3(1.0f);
    eye[itr].specular = 1.0f;
    eye[itr].c = 0;
    eye[itr].C = 0;
    eye[itr].d = 0;
    eye[itr].D = 0;

    radiance += _gather(*context.engine, eye[itr], radius);
    radiance += _connect(*context.engine, eye[itr], light_path, radius);
    std::swap(itr, prv);

    size_t path_size = 2;
    float roulette = path_size < _minSubpath ? 1.0f : _roulette;
    float uniform = sampleUniform1(*context.engine).value();

    while (uniform < roulette) {
        auto bsdf = _scene->sampleBSDF(*context.engine, eye[prv].surface, eye[prv].omega);

        isect = _scene->intersectMesh(eye[prv].surface.position(), bsdf.omega());

        if (!isect.isPresent()) {
            return radiance;
        }

        eye[itr].surface = _scene->querySurface(isect);
        eye[itr].omega = -bsdf.omega();

        auto edge = Edge(eye[prv], eye[itr]);

        eye[itr].throughput
            = eye[prv].throughput
            * bsdf.throughput()
            * edge.bCosTheta
            / (bsdf.density() * roulette);

        eye[prv].specular = max(eye[prv].specular, bsdf.specular());
        eye[itr].specular = bsdf.specular();
        eye[itr].c = 1.0f / Beta::beta(edge.fGeometry * bsdf.density());

        eye[itr].C
            = (eye[prv].C
                * Beta::beta(bsdf.densityRev())
                + eye[prv].c * (1.0f - eye[prv].specular))
            * Beta::beta(edge.bGeometry)
            * eye[itr].c;

        eye[itr].d = 1.0f;

        eye[itr].D
            = (eye[prv].D
                * Beta::beta(bsdf.densityRev())
                + eye[prv].d * (1.0f - bsdf.specular()))
            * Beta::beta(edge.bGeometry)
            * eye[itr].c;


        ++path_size;
        radiance += _gather(*context.engine, eye[itr], radius);
        radiance += _connect(*context.engine, eye[itr], light_path, radius);
        std::swap(itr, prv);

        roulette = path_size < _minSubpath ? 1.0f : _roulette;
        uniform = sampleUniform1(*context.engine).value();
    }

    return radiance;
}

template <class Beta> template <class Appender>
void UPGBase<Beta>::_traceLight(RandomEngine& engine, Appender& path) {
    size_t itr = path.size() + 1, prv = path.size();

    LightSampleEx light = _scene->sampleLight(engine);

    RayIsect isect = _scene->intersectMesh(light.position(), light.omega());

    if (!isect.isPresent()) {
        return;
    }

    auto edge = Edge(light, isect);

    size_t path_size = 2;
    path.emplace_back();
    path[prv].surface = _scene->querySurface(isect);
    path[prv].omega = -light.omega();
    path[prv].throughput = light.radiance() * edge.bCosTheta / light.density();
    path[prv].specular = 0.0f;
    path[prv].a = 1.0f / Beta::beta(edge.fGeometry * light.omegaDensity());
    path[prv].A = Beta::beta(edge.bGeometry) * path[prv].a / Beta::beta(light.areaDensity());
    path[prv].B = 0.0f;

    float roulette = path_size < _minSubpath ? 1.0f : _roulette;
    float uniform = sampleUniform1(engine).value();

    while (uniform < roulette) {
        auto bsdf = _scene->sampleBSDF(engine, path[prv].surface, path[prv].omega);

        isect = _scene->intersectMesh(path[prv].surface.position(), bsdf.omega());

        if (!isect.isPresent()) {
            break;
        }

        ++path_size;
        path.emplace_back();

        path[itr].surface = _scene->querySurface(isect);
        path[itr].omega = -bsdf.omega();

        edge = Edge(path[prv], path[itr]);

        path[itr].throughput
            = path[prv].throughput
            * bsdf.throughput()
            * edge.bCosTheta
            / (bsdf.density() * roulette);

        path[prv].specular = max(path[prv].specular, bsdf.specular());
        path[itr].specular = bsdf.specular();

        path[itr].a = 1.0f / Beta::beta(edge.fGeometry * bsdf.density());

        path[itr].A
            = (path[prv].A
                * Beta::beta(bsdf.densityRev())
                + path[prv].a * (1.0f - path[prv].specular))
            * Beta::beta(edge.bGeometry)
            * path[itr].a;

        path[itr].B
            = (path[prv].B
                * Beta::beta(bsdf.densityRev())
                + (1.0f - bsdf.specular()))
            * Beta::beta(edge.bGeometry)
            * path[itr].a;

        if (bsdf.specular() == 1.0f) {
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

    if (bsdf.specular() == 1.0f) {
        path.pop_back();
    }
}

template <class Beta>
void UPGBase<Beta>::_traceLight(
    RandomEngine& engine,
    vector<LightVertex>& path) {
    _traceLight<vector<LightVertex>>(engine, path);
}

template <class Beta>
void UPGBase<Beta>::_traceLight(
    RandomEngine& engine,
    fixed_vector<LightVertex, _maxSubpath>& path) {
    _traceLight<fixed_vector<LightVertex, _maxSubpath>>(engine, path);
}

template <class Beta>
float UPGBase<Beta>::_weightVC(
    const LightVertex& light,
    const BSDFQuery& lightBSDF,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    const Edge& edge,
    float radius) {
    float eta = float(_numPhotons) * pi<float>() * radius * radius;

    float Ap
        = (light.A * Beta::beta(lightBSDF.densityRev()) + light.a * (1.0f - light.specular))
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev());

    float Bp
        = light.B * Beta::beta(lightBSDF.densityRev())
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev());

    float Cp
        = (eye.C * Beta::beta(eyeBSDF.density()) + eye.c * (1.0f - eye.specular))
        * Beta::beta(edge.fGeometry * lightBSDF.density());

    float Dp
        = (eye.D * Beta::beta(eyeBSDF.density()) + eye.d * (1.0f - eyeBSDF.specular()))
        * Beta::beta(edge.fGeometry * lightBSDF.density());

    float weightInv
        = Ap + eta * Bp + Cp + eta * Dp
        + Beta::beta(eta * edge.bGeometry * eyeBSDF.densityRev()) + 1.0f;

    return 1.0f / weightInv;
}

template <class Beta>
float UPGBase<Beta>::_weightVM(
    const LightVertex& light,
    const BSDFQuery& lightBSDF,
    const EyeVertex& eye,
    const BSDFQuery& eyeBSDF,
    const Edge& edge,
    float radius) {
    float eta = float(_numPhotons) * pi<float>() * radius * radius;

    float weight = _weightVC(light, lightBSDF, eye, eyeBSDF, edge, radius);

    return Beta::beta(eta * edge.bGeometry * eyeBSDF.densityRev()) * weight;
}

template <class Beta>
float UPGBase<Beta>::_density(
    RandomEngine& engine,
    const LightVertex& light,
    const EyeVertex& eye,
    const BSDFQuery& eyeQuery,
    const Edge& edge,
    float radius) {
    float L = 4096.f;
    float N = 1.0f;
    const auto& eyeBSDF = _scene->queryBSDF(eye.surface);

    auto target = eye.surface.toSurface(light.surface.position() - eye.surface.position());
    auto bound = angular_bound(target, radius);
    auto omega = eye.surface.toSurface(eye.omega);

    while (N < L) {
        auto bsdfSample = eyeBSDF.sampleBounded(engine, omega, bound);

        bsdfSample._omega = eye.surface.toWorld(bsdfSample._omega);

        RayIsect isect = _scene->intersectMesh(
            eye.surface.position(),
            bsdfSample.omega());

        if (isect.isPresent()) {
            float distance_sq = distance2(light.surface.position(), isect.position());

            if (distance_sq < radius * radius) {
                return N / bsdfSample.area();
            }
        }

        N += 1.0f;
    }

    return 1.0f / (edge.bGeometry * eyeQuery.densityRev() * pi<float>() * radius * radius);
}

template <class Beta>
vec3 UPGBase<Beta>::_connect0(RandomEngine& engine, const EyeVertex& eye, float radius) {
    vec3 radiance = vec3(0.0f);

    auto bsdf = _scene->sampleBSDF(engine, eye.surface, eye.omega);
    RayIsect isect = _scene->intersect(eye.surface.position(), bsdf.omega());

    while (isect.isLight()) {
        auto edge = Edge(eye, isect, bsdf.omega());

        float eta = float(_numPhotons) * pi<float>() * radius * radius;

        auto lsdf = _scene->queryLSDF(isect, -bsdf.omega());

        float c = 1.0f / Beta::beta(edge.fGeometry * bsdf.density());

        float C
            = (eye.C * Beta::beta(bsdf.densityRev())
                + eye.c * (1.0f - max(eye.specular, bsdf.specular())))
            * Beta::beta(edge.bGeometry) * c;

        float Cp
            = (C * Beta::beta(lsdf.omegaDensity()) + c * (1.0f - bsdf.specular()))
            * Beta::beta(lsdf.areaDensity());

        float Dp
            = (eye.D * Beta::beta(bsdf.densityRev()) + eye.d * (1.0f - bsdf.specular()))
            * Beta::beta(edge.bGeometry * lsdf.omegaDensity());

        float weightInv = Cp + eta * Dp + 1.0f;

        radiance
            += _combine(
                lsdf.radiance()
                    * eye.throughput
                    * bsdf.throughput()
                    * edge.bCosTheta
                    / bsdf.density(),
                1.0f / weightInv);

        isect = _scene->intersect(isect.position(), bsdf.omega());
    }

    return radiance;
}

template <class Beta>
vec3 UPGBase<Beta>::_connect1(RandomEngine& engine, const EyeVertex& eye, float radius) {
    LightSampleEx lightSample = _scene->sampleLightEx(engine, eye.surface.position());

    auto eyeBSDF = _scene->queryBSDF(eye.surface, -lightSample.omega(), eye.omega);

    if (eyeBSDF.specular() == 1.0f) {
        return vec3(0.0f);
    }

    auto edge = Edge(lightSample, eye, lightSample.omega());

    float eta = float(_numPhotons) * pi<float>() * radius * radius;

    float Ap
        = Beta::beta(eyeBSDF.density() * edge.bGeometry / lightSample.areaDensity());

    float Cp
        = (eye.C * Beta::beta(eyeBSDF.density()) + eye.c * (1.0f - eye.specular))
        * Beta::beta(edge.fGeometry * lightSample.omegaDensity());

    float Dp
        = (eye.D * Beta::beta(eyeBSDF.density()) + eye.d)
        * Beta::beta(edge.fGeometry * lightSample.omegaDensity());

    float weightInv
        = Ap + Cp + eta * Dp + 1.0f;

    return _combine(
        lightSample.radiance()
            * eye.throughput
            * eyeBSDF.throughput()
            * edge.bCosTheta
            * edge.fGeometry
            / lightSample.areaDensity(),
        1.0f / weightInv);
}

template <class Beta>
vec3 UPGBase<Beta>::_connect(const LightVertex& light, const EyeVertex& eye, float radius) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    if (eyeBSDF.specular() == 1.0f) {
        return vec3(0.0f);
    }

    auto edge = Edge(light, eye, omega);

    auto weight = _weightVC(light, lightBSDF, eye, eyeBSDF, edge, radius);

    return _combine(
        _scene->occluded(eye.surface.position(), light.surface.position())
            * light.throughput
            * lightBSDF.throughput()
            * eye.throughput
            * eyeBSDF.throughput()
            * edge.bCosTheta
            * edge.fGeometry,
        weight);
}

template <class Beta>
vec3 UPGBase<Beta>::_connect(
    RandomEngine& engine,
    const EyeVertex& eye,
    const fixed_vector<LightVertex, _maxSubpath>& path,
    float radius) {
    vec3 radiance = _connect0(engine, eye, radius) + _connect1(engine, eye, radius);

    for (size_t i = 0; i < path.size(); ++i) {
        radiance += _connect(path[i], eye, radius);
    }

    return radiance;
}

template <class Beta>
void UPGBase<Beta>::_scatter(RandomEngine& engine) {
    vector<LightVertex> vertices;

    for (size_t i = 0; i < _numPhotons; ++i) {
        _traceLight(engine, vertices);
    }

    _vertices = v3::HashGrid3D<LightVertex>(move(vertices), _radius);
}

template <class Beta>
vec3 UPGBase<Beta>::_gather(RandomEngine& engine, const EyeVertex& eye, float& radius) {
    auto eyeBSDF = _scene->sampleBSDF(engine, eye.surface, eye.omega);
    RayIsect isect = _scene->intersectMesh(eye.surface.position(), eyeBSDF.omega());

    if (!isect.isPresent()) {
        return vec3(0.0f);
    }

    vec3 radiance = vec3(0.0f);

    radius = _radius;

    _vertices.rQuery(
        [&](const LightVertex& light) {
            radiance += _merge(engine, light, eye, radius);
        },
        isect.position(),
        radius);

    return radiance / float(_numPhotons);
}

template <class Beta>
vec3 UPGBase<Beta>::_merge(
    RandomEngine& engine,
    const LightVertex& light,
    const EyeVertex& eye,
    float radius) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    if (eyeBSDF.specular() == 1.0f) {
        return vec3(0.0f);
    }

    auto edge = Edge(light, eye, omega);

    auto weight = _weightVM(light, lightBSDF, eye, eyeBSDF, edge, radius);
    auto density = _density(engine, light, eye, eyeBSDF, edge, radius);

    vec3 result = _scene->occluded(eye.surface.position(), light.surface.position())
        * light.throughput
        * lightBSDF.throughput()
        * eye.throughput
        * eyeBSDF.throughput()
        * edge.bCosTheta
        * edge.fGeometry
        * density;

    if (result.x + result.y + result.z != 0.0f) {
        return _combine(result, weight);
    }
    else {
        return _combine(vec3(0.0f), weight);
    }
}

template <class Beta>
vec3 UPGBase<Beta>::_combine(vec3 throughput, float weight) {
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
    : UPGBase<VariableBeta>(
        minSubpath,
        roulette,
        numPhotons,
        radius,
        numGather,
        num_threads)
{
    VariableBeta::init(beta);
}

template class UPGBase<FixedBeta<0>>;
template class UPGBase<FixedBeta<1>>;
template class UPGBase<FixedBeta<2>>;
template class UPGBase<VariableBeta>;


}
