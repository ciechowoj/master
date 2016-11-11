#include <BPT.hpp>
#include <Edge.hpp>
#include <iostream>

namespace haste {

template <class Beta> BPTBase<Beta>::BPTBase(size_t minSubpath, float roulette)
    : _minSubpath(minSubpath)
    , _roulette(roulette)
{ }

template <class Beta> string BPTBase<Beta>::name() const {
    return Beta::name();
}

template <class Beta> vec3 BPTBase<Beta>::_traceEye(
    RandomEngine& engine,
    const Ray& ray)
{
    char lightRaw[_maxSubpath * sizeof(LightVertex)];
    LightVertex* light = (LightVertex*)lightRaw;

    size_t lSize = 0;

    _trace(engine, lSize, light);

    vec3 radiance = vec3(0.0f);
    EyeVertex eye[2];
    size_t itr = 0, prv = 1;

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
    eye[itr].throughput = vec3(1.0f);
    eye[itr].specular = 1.0f;
    eye[itr].c = 0;
    eye[itr].C = 0;

    radiance += _connect(engine, eye[itr], lSize, light);
    std::swap(itr, prv);

    size_t eSize = 2;
    float roulette = eSize < _minSubpath ? 1.0f : _roulette;
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

        ++eSize;
        radiance += _connect(engine, eye[itr], lSize, light);
        std::swap(itr, prv);

        roulette = eSize < _minSubpath ? 1.0f : _roulette;
        uniform = sampleUniform1(engine).value();
    }

    return radiance;
}

template <class Beta> void BPTBase<Beta>::_trace(
    RandomEngine& engine,
    size_t& size,
    LightVertex* path)
{
    size_t itr = 0, prv = 0;

    LightSampleEx light = _scene->sampleLight(engine);

    RayIsect isect = _scene->intersectMesh(light.position(), light.omega());

    if (!isect.isPresent()) {
        size = 0;
        return;
    }

    auto edge = Edge(light, isect);

    path[itr].surface = _scene->querySurface(isect);
    path[itr].omega = -light.omega();
    path[itr].throughput = light.radiance() * edge.bCosTheta / light.density();
    path[itr].specular = 0.0f;

    path[itr].a = 1.0f / Beta::beta(edge.fGeometry * light.omegaDensity());
    path[itr].A = Beta::beta(edge.bGeometry) * path[itr].a / Beta::beta(light.areaDensity());

    prv = itr;
    ++itr;

    size_t lSize = 2;
    float roulette = lSize < _minSubpath ? 1.0f : _roulette;
    float uniform = sampleUniform1(engine).value();

    while (uniform < roulette) {
        auto bsdf = _scene->sampleBSDF(engine, path[prv].surface, path[prv].omega);

        isect = _scene->intersectMesh(path[prv].surface.position(), bsdf.omega());

        if (!isect.isPresent()) {
            break;
        }

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

        if (bsdf.specular() == 1.0f) {
            path[prv] = path[itr];
        }
        else {
            prv = itr;
            ++itr;
        }

        ++lSize;
        roulette = lSize < _minSubpath ? 1.0f : _roulette;
        uniform = sampleUniform1(engine).value();
    }

    auto bsdf = _scene->sampleBSDF(
        engine,
        path[prv].surface,
        path[prv].omega);

    if (bsdf.specular() == 1.0f) {
        size = prv;
    }
    else {
        size = prv + 1;
    }
}

template <class Beta> vec3 BPTBase<Beta>::_connect0(
    RandomEngine& engine,
    const EyeVertex& eye)
{
    vec3 radiance = vec3(0.0f);

    auto bsdf = _scene->sampleBSDF(engine, eye.surface, eye.omega);
    RayIsect isect = _scene->intersect(eye.surface.position(), bsdf.omega());

    while (isect.isLight()) {
        auto edge = Edge(eye, isect, bsdf.omega());
        auto lsdf = _scene->queryLSDF(isect, -bsdf.omega());

        float c = 1.0f / Beta::beta(edge.fGeometry * bsdf.density());

        float C
            = (eye.C * Beta::beta(bsdf.densityRev())
                + eye.c * (1.0f - max(eye.specular, bsdf.specular())))
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

template <class Beta> vec3 BPTBase<Beta>::_connect1(
    RandomEngine& engine,
    const EyeVertex& eye)
{
    LightSampleEx light = _scene->sampleLightEx(engine, eye.surface.position());

    auto bsdf = _scene->queryBSDF(eye.surface, -light.omega(), eye.omega);

    if (bsdf.specular() == 1.0f) {
        return vec3(0.0f);
    }

    auto edge = Edge(light, eye, light.omega());

    float Ap = Beta::beta(bsdf.density() * edge.bGeometry / light.areaDensity());

    float Cp
        = (eye.C * Beta::beta(bsdf.density()) + eye.c * (1.0f - eye.specular))
        * Beta::beta(edge.fGeometry * light.omegaDensity());

    float weightInv = Ap + Cp + 1.0f;

    return light.radiance()
        * eye.throughput
        * bsdf.throughput()
        * edge.bCosTheta
        * edge.fGeometry
        / (light.areaDensity() * weightInv);
}

template <class Beta> vec3 BPTBase<Beta>::_connect(
    const EyeVertex& eye,
    const LightVertex& light)
{
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

    float Cp
        = (eye.C * Beta::beta(eyeBSDF.density()) + eye.c * (1.0f - eye.specular))
        * Beta::beta(edge.fGeometry * lightBSDF.density());

    float weightInv = Ap + Cp + 1.0f;

    return _scene->occluded(eye.surface.position(), light.surface.position())
        * light.throughput
        * lightBSDF.throughput()
        * eye.throughput
        * eyeBSDF.throughput()
        * edge.bCosTheta
        * edge.fGeometry
        / weightInv;
}

template <class Beta> vec3 BPTBase<Beta>::_connect(
    RandomEngine& engine,
    const EyeVertex& eye,
    size_t size,
    const LightVertex* path)
{
    vec3 radiance = _connect0(engine, eye) + _connect1(engine, eye);

    for (size_t i = 0; i < size; ++i) {
        radiance += _connect(eye, path[i]);
    }

    return radiance;
}

BPTb::BPTb(size_t minSubpath, float roulette, float beta)
    : BPTBase<VariableBeta>(minSubpath, roulette)
{
    VariableBeta::init(beta);
}

template class BPTBase<FixedBeta<0>>;
template class BPTBase<FixedBeta<1>>;
template class BPTBase<FixedBeta<2>>;
template class BPTBase<VariableBeta>;

}
