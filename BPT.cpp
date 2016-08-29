#include <BPT.hpp>

namespace haste {

BPT::BPT(size_t minSubpath, float roulette)
    : _minSubpath(minSubpath)
    , _roulette(roulette)
{ }

string BPT::name() const {
    return "Bidirectional Path Tracing (Balance Heuristic)";
}

vec3 BPT::_trace(RandomEngine& engine, const Ray& ray) {
    char lightRaw[_maxSubpath * sizeof(LightVertex)];
    LightVertex* light = (LightVertex*)lightRaw;

    size_t lSize = 0;

    _trace(engine, lSize, light);

    vec3 radiance = vec3(0.0f);
    EyeVertex eye[2];
    size_t itr = 0, prv = 1;

    RayIsect isect = _scene->intersect(ray.origin, ray.direction);

    while (isect.isLight()) {
        radiance += _scene->queryRadiance(isect);
        isect = _scene->intersect(isect.position(), ray.direction);
    }

    if (!isect.isPresent()) {
        return radiance;
    }

    eye[itr].surface = _scene->querySurface(isect);
    eye[itr]._omega = -ray.direction;
    eye[itr].throughput = vec3(1.0f);
    eye[itr].specular = 0.0f;
    eye[itr].c = 0;
    eye[itr].C = 0;

    size_t eSize = 2;

    radiance += _connect(engine, eye[itr], lSize, light);
    std::swap(itr, prv);

    float roulette = eSize < _minSubpath ? 1.0f : _roulette;
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

        eye[itr].throughput =
            eye[prv].throughput *
            bsdf.throughput() *
            edge.bCosTheta /
            (bsdf.density() * roulette);

        eye[itr].specular = max(eye[prv].specular, bsdf.specular()) * bsdf.specular();
        eye[itr].c = 1.0f / (edge.fGeometry * bsdf.density());
        eye[itr].C =
            (eye[prv].C * bsdf.densityRev() + eye[prv].c * (1.0f - eye[prv].specular)) *
            edge.bGeometry *
            eye[itr].c;

        ++eSize;
        radiance += _connect(engine, eye[itr], lSize, light);
        std::swap(itr, prv);

        roulette = eSize < _minSubpath ? 1.0f : _roulette;
        uniform = sampleUniform1(engine).value();
    }

    return radiance;
}

void BPT::_trace(RandomEngine& engine, size_t& size, LightVertex* path) {
    size_t itr = 0, prv = 0;

    LightSampleEx light = _scene->sampleLight(engine);

    RayIsect isect = _scene->intersectMesh(light.position(), light.omega());

    if (!isect.isPresent()) {
        size = 0;
        return;
    }

    float specular = 0.0f;

    auto edge = Edge(light, isect);

    path[itr].surface = _scene->querySurface(isect);
    path[itr]._omega = -light.omega();
    path[itr].throughput = light.radiance() * edge.bCosTheta / light.density();
    path[itr].a = 1.0f / (edge.fGeometry * light.omegaDensity());
    path[itr].A = edge.bGeometry * path[itr].a / light.areaDensity();

    prv = itr;
    ++itr;

    size_t lSize = 2;
    float roulette = lSize < _minSubpath ? 1.0f : _roulette;
    float uniform = sampleUniform1(engine).value();

    while (uniform < roulette) {
        auto bsdf = _scene->sampleAdjointBSDF(engine, path[prv].surface, path[prv].omega());

        isect = _scene->intersectMesh(path[prv].position(), bsdf.omega());

        if (!isect.isPresent()) {
            break;
        }

        path[itr].surface = _scene->querySurface(isect);
        path[itr]._omega = -bsdf.omega();

        edge = Edge(path[prv], path[itr]);

        path[itr].throughput =
            path[prv].throughput *
            bsdf.throughput() *
            edge.bCosTheta /
            (bsdf.density() * roulette);

        path[itr].a = 1.0f / (edge.fGeometry * bsdf.density());
        path[itr].A =
            (path[prv].A * bsdf.densityRev() + path[prv].a * (1.0f - specular)) *
            edge.bGeometry *
            path[itr].a;

        specular = max(specular, bsdf.specular()) * bsdf.specular();

        if (bsdf.specular() > 0.0f) {
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

    auto bsdf = _scene->sampleAdjointBSDF(engine, path[prv].surface, path[prv].omega());

    if (bsdf.specular() > 0.0f) {
        size = prv;
    }
    else {
        size = prv + 1;
    }
}

vec3 BPT::_connect0(RandomEngine& engine, const EyeVertex& eye) {
    vec3 radiance = vec3(0.0f);

    auto bsdf = _scene->sampleBSDF(engine, eye.surface, eye.omega());
    RayIsect isect = _scene->intersect(eye.position(), bsdf.omega());

    float roulette = 1.0f;

    while (isect.isLight())
    {
        if (eye.specular * bsdf.specular() > 0.0f)
        {
            radiance +=
                _scene->queryRadiance(isect, -bsdf.omega()) *
                eye.throughput *
                bsdf.throughput() *
                abs(dot(eye.gnormal(), bsdf.omega())) /
                (bsdf.density() * roulette);
        }
        else
        {
            auto edge = Edge(eye, isect, bsdf.omega());

            auto lsdf = _scene->queryLSDF(isect, -bsdf.omega());
            float c = 1.0f / (edge.fGeometry * bsdf.density());
            float C = (eye.C * bsdf.densityRev() + eye.c * (1.0f - eye.specular)) * edge.bGeometry * c;

            float weightInv = 1.0f + (C * lsdf.omegaDensity() + c) * lsdf.areaDensity();

            radiance +=
                lsdf.radiance() *
                eye.throughput *
                bsdf.throughput() *
                edge.bCosTheta /
                (bsdf.density() * roulette * weightInv);
        }

        isect = _scene->intersect(isect.position(), bsdf.omega());
    }

    return radiance;
}

vec3 BPT::_connect1(RandomEngine& engine, const EyeVertex& eye) {
    LightSampleEx light = _scene->sampleLightEx(engine, eye.position());
    auto bsdf = _scene->queryBSDF(eye.surface, -light.omega(), eye.omega());

    auto edge = Edge(light, eye, light.omega());

    float weightInv =
        bsdf.densityRev() * edge.bGeometry / light.areaDensity() +
        1.0f +
        (eye.C * bsdf.density() + eye.c * (1.0f - eye.specular)) * edge.fGeometry * light.omegaDensity();

    return
        light.radiance() *
        eye.throughput *
        bsdf.throughput() *
        edge.bCosTheta *
        edge.fGeometry /
        (light.areaDensity() * weightInv);
}

vec3 BPT::_connect(const EyeVertex& eye, const LightVertex& light) {
    vec3 omega = normalize(eye.position() - light.position());

    auto lightBSDF = _scene->queryAdjointBSDF(light.surface, light.omega(), omega);
    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega());

    auto edge = Edge(light, eye, omega);

    float weightInv =
        (light.A * lightBSDF.densityRev() + light.a) * edge.bGeometry * eyeBSDF.densityRev() +
        1.0f +
        (eye.C * eyeBSDF.density() + eye.c) * edge.fGeometry * lightBSDF.density();

    return
        _scene->occluded(eye.position(), light.position()) *
        light.throughput *
        lightBSDF.throughput() *
        eye.throughput *
        eyeBSDF.throughput() *
        edge.bCosTheta *
        edge.fGeometry /
        weightInv;
}

vec3 BPT::_connect(
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

}
