#include <UPG.hpp>
#include <Edge.hpp>

namespace haste {

template <class Beta>
UPGBase<Beta>::UPGBase(
    size_t minSubpath,
    float roulette,
    size_t numPhotons,
    size_t numGather)
    : _minSubpath(minSubpath)
    , _roulette(roulette)
    , _numPhotons(numPhotons)
    , _numGather(numGather)
{ }

template <class Beta>
string UPGBase<Beta>::name() const {
    return "Unbiased Photon Gathering";
}

template <class Beta>
void UPGBase<Beta>::_updateWeights(
    EyeVertex& current,
    const EyeVertex& previous,
    const BSDFSample& bsdf,
    const Edge& edge) {

}

template <class Beta>
void UPGBase<Beta>::_updateWeights(
    LightVertex& current,
    const LightVertex& previous,
    const BSDFSample& bsdf,
    const Edge& edge) {

}

template <class Beta>
vec3 UPGBase<Beta>::_traceEye(RandomEngine& engine, const Ray& ray) {
    /* vec3 radiance = vec3(0.0f);

    RayIsect isect = _scene->intersect(ray.origin, ray.direction);

    while (isect.isLight()) {
        radiance += _scene->queryRadiance(isect, -ray.direction);
        isect = _scene->intersect(isect.position(), ray.direction);
    }

    if (!isect.isPresent()) {
        return radiance;
    }

    fixed_vector<LightVertex, _maxSubpath> light_path;
    _traceLight(engine, light_path);

    EyeVertex eye[2];
    size_t itr = 0, prv = 1;

    eye[itr].surface = _scene->querySurface(isect);
    eye[itr]._omega = -ray.direction;
    eye[itr].throughput = vec3(1.0f);
    eye[itr].specular = 1.0f;
    eye[itr].c = 0;
    eye[itr].C = 0;

    radiance += _connect(engine, eye[itr], light_path);
    radiance += _gather(engine, eye[itr]);
    std::swap(itr, prv);

    size_t path_size = 2;
    float roulette = path_size < _minSubpath ? 1.0f : _roulette;
    float uniform = sampleUniform1(engine).value();

    while (uniform < roulette) {
        auto bsdf = _scene->sampleBSDF(engine, eye[prv].surface, eye[prv].omega());

        isect = _scene->intersectMesh(eye[prv].position(), bsdf.omega());

        if (!isect.isPresent()) {
            return radiance;
        }

        eye[itr].surface = _scene->querySurface(isect);
        eye[itr]._omega = -bsdf.omega();

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
                + eye[prv].c * (1.0f - eye[prv].specular)
                + Beta::beta(_eta) * (1.0f - bsdf.specular()))
            * Beta::beta(edge.bGeometry)
            * eye[itr].c;

        ++path_size;
        radiance += _connect(engine, eye[itr], path_size, light_path);
        radiance += _gather(engine, eye[itr]);
        std::swap(itr, prv);

        roulette = path_size < _minSubpath ? 1.0f : _roulette;
        uniform = sampleUniform1(engine).value();
    }

    return radiance; */
}

template <class Beta> template <class Appender>
void UPGBase<Beta>::_traceLight(RandomEngine& engine, Appender& path) {
    size_t itr = 0, prv = 0;

    LightSampleEx light = _scene->sampleLight(engine);

    RayIsect isect = _scene->intersectMesh(light.position(), light.omega());

    if (!isect.isPresent()) {
        return;
    }

    auto edge = Edge(light, isect);

    path.emplace_back();
    path[itr].surface = _scene->querySurface(isect);
    path[itr].omega = -light.omega();
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

        isect = _scene->intersectMesh(path[prv].surface.position(), bsdf.omega());

        if (!isect.isPresent()) {
            break;
        }

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
                + Beta::beta(_eta) * (1.0f - bsdf.specular()))
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

        ++path_size;
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
vec3 UPGBase<Beta>::_connect0(RandomEngine& engine, const EyeVertex& eye) {

}

template <class Beta>
vec3 UPGBase<Beta>::_connect1(RandomEngine& engine, const EyeVertex& eye) {

}

template <class Beta>
vec3 UPGBase<Beta>::_connect(const LightVertex& light, const EyeVertex& eye) {

}

template <class Beta>
vec3 UPGBase<Beta>::_connect(
    RandomEngine& engine,
    const EyeVertex& eye,
    const fixed_vector<LightVertex, _maxSubpath>& path) {

}

template <class Beta>
void UPGBase<Beta>::_scatter(RandomEngine& engine) {

}


template <class Beta>
vec3 UPGBase<Beta>::_gather(RandomEngine& engine, const EyeVertex& eye) {

}

template <class Beta>
vec3 UPGBase<Beta>::_merge(const LightVertex& light, const EyeVertex& eye, float radius) {

}

UPGb::UPGb(
    size_t minSubpath,
    float roulette,
    size_t numPhotons,
    size_t numGather,
    float beta)
    : UPGBase<VariableBeta>(
        minSubpath,
        roulette,
        numPhotons,
        numGather)
{
    VariableBeta::init(beta);
}

template class UPGBase<FixedBeta<0>>;
template class UPGBase<FixedBeta<1>>;
template class UPGBase<FixedBeta<2>>;
template class UPGBase<VariableBeta>;


}
