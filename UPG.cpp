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
vec3 UPGBase<Beta>::_traceEye(RandomEngine& engine, const Ray& ray) {
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
    _traceLight(engine, light_path);

    EyeVertex eye[2];
    size_t itr = 0, prv = 1;
    float d = 0.0f;

    eye[itr].surface = _scene->querySurface(isect);
    eye[itr].omega = -ray.direction;
    eye[itr].throughput = vec3(1.0f);
    eye[itr].specular = 1.0f;
    eye[itr].c = 0;
    eye[itr].C = 0;
    eye[itr].D = 0;

    radiance += _connect(engine, eye[itr], light_path);
    radiance += _gather(engine, eye[itr]);
    std::swap(itr, prv);

    size_t path_size = 2;
    float roulette = path_size < _minSubpath ? 1.0f : _roulette;
    float uniform = sampleUniform1(engine).value();

    while (uniform < roulette) {
        auto bsdf = _scene->sampleBSDF(engine, eye[prv].surface, eye[prv].omega);

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

        eye[itr].D
            = (eye[prv].D
                * Beta::beta(bsdf.densityRev())
                + d * (1.0f - bsdf.specular()))
            * Beta::beta(edge.bGeometry)
            * eye[itr].c;

        d = 1.0f;

        ++path_size;
        radiance += _connect(engine, eye[itr], light_path);
        radiance += _gather(engine, eye[itr]);
        std::swap(itr, prv);

        roulette = path_size < _minSubpath ? 1.0f : _roulette;
        uniform = sampleUniform1(engine).value();
    }

    return radiance;
}

template <class Beta> template <class Appender>
void UPGBase<Beta>::_traceLight(RandomEngine& engine, Appender& path) {
    size_t itr = 1, prv = 0;

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
vec3 UPGBase<Beta>::_connect0(RandomEngine& engine, const EyeVertex& eye) {
    vec3 radiance = vec3(0.0f);

    auto bsdf = _scene->sampleBSDF(engine, eye.surface, eye.omega);
    RayIsect isect = _scene->intersect(eye.surface.position(), bsdf.omega());

    while (isect.isLight()) {
        auto edge = Edge(eye, isect, bsdf.omega());
        auto lsdf = _scene->queryLSDF(isect, -bsdf.omega());

        float c = 1.0f / Beta::beta(edge.fGeometry * bsdf.density());

        float C
            = (eye.C * Beta::beta(bsdf.densityRev())
                + eye.c * (1.0f - max(eye.specular, bsdf.specular()))
                + Beta::beta(_eta) * (1.0f - bsdf.specular()))
            * Beta::beta(edge.bGeometry) * c;

        float Cp
            = (C * Beta::beta(lsdf.omegaDensity()) + c * (1.0f - bsdf.specular()))
            * Beta::beta(lsdf.areaDensity());

        float weightInv = Cp + 1.0f;

        radiance
            += lsdf.radiance()
            * eye.throughput
            * bsdf.throughput()
            * edge.bCosTheta
            / (bsdf.density() * weightInv);

        isect = _scene->intersect(isect.position(), bsdf.omega());
    }

    return radiance;
}

template <class Beta>
vec3 UPGBase<Beta>::_connect1(RandomEngine& engine, const EyeVertex& eye) {
    LightSampleEx light = _scene->sampleLightEx(engine, eye.surface.position());

    auto bsdf = _scene->queryBSDF(eye.surface, -light.omega(), eye.omega);

    if (bsdf.specular() == 1.0f) {
        return vec3(0.0f);
    }

    auto edge = Edge(light, eye, light.omega());

    float Ap = Beta::beta(bsdf.densityRev() * edge.bGeometry / light.areaDensity());

    float Bp = 0.0f;

    float Cp
        = (eye.C * Beta::beta(bsdf.density()) + eye.c * (1.0f - eye.specular))
        * Beta::beta(edge.fGeometry * light.omegaDensity());

    float weightInv = Ap + Bp + Cp + Beta::beta(_eta * edge.fGeometry * light.omegaDensity()) + 1.0f;

    return light.radiance()
        * eye.throughput
        * bsdf.throughput()
        * edge.bCosTheta
        * edge.fGeometry
        / (light.areaDensity() * weightInv);
}

template <class Beta>
vec3 UPGBase<Beta>::_connect(const LightVertex& light, const EyeVertex& eye) {
    vec3 omega = normalize(eye.surface.position() - light.surface.position());

    auto lightBSDF = _scene->queryBSDF(light.surface, light.omega, omega);
    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    if (eyeBSDF.specular() == 1.0f) {
        return vec3(0.0f);
    }

    auto edge = Edge(light, eye, omega);

    float Ap
        = (light.A * Beta::beta(lightBSDF.densityRev()) + light.a * (1.0f - light.specular))
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev());

    float Bp
        = (light.B * Beta::beta(lightBSDF.densityRev()) + Beta::beta(_eta) * (1.0f - lightBSDF.specular()))
        * Beta::beta(edge.bGeometry * eyeBSDF.densityRev());

    float Cp
        = (eye.C * Beta::beta(eyeBSDF.density()) + eye.c * (1.0f - eye.specular))
        * Beta::beta(edge.fGeometry * lightBSDF.density());

    float weightInv = Ap + Bp + Cp + Beta::beta(_eta * edge.fGeometry * lightBSDF.density()) + 1.0f;

    return _scene->occluded(eye.surface.position(), light.surface.position())
        * light.throughput
        * lightBSDF.throughput()
        * eye.throughput
        * eyeBSDF.throughput()
        * edge.bCosTheta
        * edge.fGeometry
        / weightInv;
}

template <class Beta>
vec3 UPGBase<Beta>::_connect(
    RandomEngine& engine,
    const EyeVertex& eye,
    const fixed_vector<LightVertex, _maxSubpath>& path) {
    vec3 radiance = _connect0(engine, eye) + _connect1(engine, eye);

    for (size_t i = 0; i < path.size(); ++i) {
        radiance += _connect(path[i], eye);
    }

    return radiance;
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
