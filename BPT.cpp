#include <BPT.hpp>

namespace haste {

BPT::BPT(size_t minSubpath, float roulette)
    : _minSubpath(minSubpath)
    , _roulette(roulette)
{ }

void BPT::render(
    ImageView& view,
    RandomEngine& engine,
    size_t cameraId)
{
    auto trace = [&](RandomEngine& engine, Ray ray) -> vec3 {
        return _trace(engine, ray);
    };

    for_each_ray(view, engine, _scene->cameras(), cameraId, trace);
}

string BPT::name() const {
    return "Bidirectional Path Tracing (Balance Heuristic)";
}

void BPT::_trace(RandomEngine& engine, size_t& size, LightVertex* path) {
    size_t itr = 0, prv = 0;

    LightSampleEx light = _scene->sampleLight(engine);

    RayIsect isect = _scene->intersectMesh(light.position(), light.omega());

    if (!isect.isPresent()) {
        size = 0;
        return;
    }

    float distSqInv = 1.0f / distance2(light.position(), isect.position());
    float fCosTheta = abs(dot(light.omega(), isect.gnormal()));
    float bCosTheta = abs(dot(light.omega(), light.normal()));
    float fgeometry = distSqInv * fCosTheta;
    float bgeometry = distSqInv * bCosTheta;

    path[itr].surface = _scene->querySurface(isect);
    path[itr].omega = -light.omega();
    path[itr].throughput = light.radiance() * bCosTheta / light.density();
    path[itr].a = 1.0f / (fgeometry * light.omegaDensity());
    path[itr].A = bgeometry * path[itr].a / light.areaDensity();

    prv = itr;
    ++itr;

    size_t lSize = 2;
    float roulette = lSize < _minSubpath ? 1.0f : _roulette;
    float uniform = sampleUniform1(engine).value();

    while (uniform < roulette) {
        auto bsdf = _scene->sampleBSDF(engine, path[prv].surface, path[prv].omega);

        isect = _scene->intersectMesh(path[prv].position(), bsdf.omega());

        if (!isect.isPresent()) {
            break;
        }

        path[itr].surface = _scene->querySurface(isect);
        path[itr].omega = -bsdf.omega();

        distSqInv = 1.0f / distance2(path[prv].position(), isect.position());
        fCosTheta = abs(dot(path[itr].omega, path[itr].gnormal()));
        bCosTheta = abs(dot(bsdf.omega(), path[prv].gnormal()));
        fgeometry = distSqInv * fCosTheta;
        bgeometry = distSqInv * bCosTheta;

        path[itr].throughput =
            path[prv].throughput *
            bsdf.throughput() *
            bCosTheta /
            (bsdf.density() * roulette);

        path[itr].a = 1.0f / (fgeometry * bsdf.density());
        path[itr].A = (path[prv].A * bsdf.densityRev() + path[prv].a) * bgeometry * path[itr].a;

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

    auto bsdf = _scene->sampleBSDF(engine, path[prv].surface, path[prv].omega);

    if (bsdf.specular() > 0.0f) {
        size = prv;
    }
    else {
        size = prv + 1;
    }
}

vec3 BPT::_trace(RandomEngine& engine, const Ray& ray) {
    LightVertex light[_maxSubpath];
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

    float distSqInv, fgeometry, bgeometry, fCosTheta, bCosTheta;

    eye[itr].surface = _scene->querySurface(isect);
    eye[itr].omega = -ray.direction;

    distSqInv = 1.0f / distance2(ray.origin, isect.position());
    fCosTheta = abs(dot(eye[itr].omega, eye[itr].gnormal()));
    fgeometry = distSqInv * fCosTheta;

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

        isect = _scene->intersectMesh(eye[prv].position(), bsdf.omega());

        if (!isect.isPresent()) {
            return radiance;
        }

        eye[itr].surface = _scene->querySurface(isect);
        eye[itr].omega = -bsdf.omega();

        distSqInv = 1.0f / distance2(eye[prv].position(), isect.position());
        fCosTheta = abs(dot(eye[itr].omega, eye[itr].gnormal()));
        bCosTheta = abs(dot(bsdf.omega(), eye[prv].gnormal()));
        fgeometry = distSqInv * fCosTheta;
        bgeometry = distSqInv * bCosTheta;

        eye[itr].throughput =
            eye[prv].throughput *
            bsdf.throughput() *
            bCosTheta /
            (bsdf.density() * roulette);

        eye[itr].specular = eye[prv].specular * bsdf.specular();
        eye[itr].c = 1.0f / (fgeometry * bsdf.density());
        eye[itr].C =
            (eye[prv].C * bsdf.densityRev() + eye[prv].c) *
            bgeometry *
            eye[itr].c;

        ++eSize;
        radiance += _connect(engine, eye[itr], lSize, light);
        std::swap(itr, prv);

        roulette = eSize < _minSubpath ? 1.0f : _roulette;
        uniform = sampleUniform1(engine).value();
    }

    return radiance;
}

vec3 BPT::_connect0(RandomEngine& engine, const EyeVertex& eye) {
    vec3 radiance = vec3(0.0f);

    auto bsdf = _scene->sampleBSDF(engine, eye.surface, eye.omega);
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
            float distSqInv = 1.0f / distance2(eye.position(), isect.position());
            float fCosTheta = abs(dot(-bsdf.omega(), isect.gnormal()));
            float bCosTheta = abs(dot(bsdf.omega(), eye.gnormal()));
            float fgeometry = distSqInv * fCosTheta;
            float bgeometry = distSqInv * bCosTheta;

            auto lsdf = _scene->queryLSDF(isect, -bsdf.omega());
            float c = 1.0f / (fgeometry * bsdf.density());
            float C = (eye.C * bsdf.densityRev() + eye.c) * bgeometry * c;

            float weightInv = 1.0f + (C * lsdf.omegaDensity() + c) * lsdf.areaDensity();

            radiance +=
                lsdf.radiance() *
                eye.throughput *
                bsdf.throughput() *
                bCosTheta /
                (bsdf.density() * roulette * weightInv);
        }

        isect = _scene->intersect(isect.position(), bsdf.omega());
    }

    return radiance;
}

vec3 BPT::_connect1(RandomEngine& engine, const EyeVertex& eye) {
    LightSampleEx light = _scene->sampleLightEx(engine, eye.position());
    auto bsdf = _scene->queryBSDFEx(eye.surface, -light.omega(), eye.omega);

    float distSqInv = 1.0f / distance2(eye.position(), light.position());
    float eCosTheta = abs(dot(light.omega(), eye.gnormal()));
    float lCosTheta = abs(dot(light.omega(), light.normal()));

    float weightInv =
        bsdf.densityRev() * lCosTheta * distSqInv / light.areaDensity() +
        1.0f +
        (eye.C * bsdf.density() + eye.c) * eCosTheta * distSqInv * light.omegaDensity();

    return
        light.radiance() *
        eye.throughput *
        bsdf.throughput() *
        eCosTheta *
        lCosTheta *
        distSqInv /
        (light.areaDensity() * weightInv);
}

vec3 BPT::_connect(const EyeVertex& eye, const LightVertex& light) {
    vec3 omega = normalize(eye.position() - light.position());

    auto lightBSDF = _scene->queryBSDFEx(light.surface, light.omega, omega);
    auto eyeBSDF = _scene->queryBSDFEx(eye.surface, -omega, eye.omega);

    float distSqInv = 1.0f / distance2(eye.position(), light.position());
    float lCosTheta = abs(dot(omega, light.normal()));
    float eCosTheta = abs(dot(-omega, eye.gnormal()));
    float lGeometry = lCosTheta * distSqInv;
    float eGeometry = eCosTheta * distSqInv;

    float weightInv =
        (light.A * lightBSDF.densityRev() + light.a) * lGeometry * eyeBSDF.densityRev() +
        1.0f +
        (eye.C * eyeBSDF.density() + eye.c) * eGeometry * lightBSDF.density();

    return
        _scene->occluded(eye.position(), light.position()) *
        light.throughput *
        lightBSDF.throughput() *
        eye.throughput *
        eyeBSDF.throughput() *
        eCosTheta *
        lCosTheta *
        distSqInv /
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
