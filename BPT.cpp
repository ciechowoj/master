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
    return "Bidirectional Path Tracing";
}

void BPT::_trace(RandomEngine& engine, size_t& size, LightVertex* path)
{
    size = 0;
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

    eye[itr].throughput = vec3(fgeometry);
    eye[itr].density = fgeometry;
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

        isect = _scene->intersect(eye[prv].position(), bsdf.omega());

        while (isect.isLight())
        {
            if (eye[prv].specular * bsdf.specular() > 0.0f)
            {
                radiance +=
                    _scene->queryRadiance(isect, -bsdf.omega()) *
                    eye[prv].throughput *
                    bsdf.throughput() *
                    abs(dot(eye[prv].gnormal(), bsdf.omega())) /
                    (eye[prv].density * bsdf.density() * roulette);
            }
            else
            {
                distSqInv = 1.0f / distance2(eye[prv].position(), isect.position());
                fCosTheta = abs(dot(-bsdf.omega(), isect.gnormal()));
                bCosTheta = abs(dot(bsdf.omega(), eye[prv].gnormal()));
                fgeometry = distSqInv * fCosTheta;
                bgeometry = distSqInv * bCosTheta;

                auto lsdf = _scene->queryLSDF(isect, -bsdf.omega());
                float c = 1.0f / (fgeometry * bsdf.density());
                float C = (eye[prv].C * bsdf.density() + eye[prv].c) * bgeometry * c;

                float weight = 1.0f / (1.0f + (C * lsdf.omegaDensity() + c) * lsdf.areaDensity());

                radiance +=
                    weight *
                    lsdf.radiance() *
                    eye[prv].throughput *
                    bsdf.throughput() *
                    bCosTheta /
                    (eye[prv].density * bsdf.density() * roulette);
            }

            isect = _scene->intersect(isect.position(), bsdf.omega());
        }

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

        eye[itr].throughput = eye[prv].throughput * bsdf.throughput() * bCosTheta * fgeometry;
        eye[itr].density = eye[prv].density * bsdf.density() * fgeometry;
        eye[itr].specular = eye[prv].specular * bsdf.specular();
        eye[itr].c = 1.0f / (fgeometry * bsdf.density());
        eye[itr].C = (eye[prv].C * bsdf.density() + eye[prv].c) * bgeometry * eye[itr].c;

        radiance += _connect(engine, eye[itr], lSize, light);
        std::swap(itr, prv);

        ++eSize;
        roulette = eSize < _minSubpath ? 1.0f : _roulette;
        uniform = sampleUniform1(engine).value();
    }

    return radiance;
}

vec3 BPT::_connect(RandomEngine& engine, const EyeVertex& eye) {
    LightSampleEx light = _scene->sampleLightEx(engine, eye.position());
    auto bsdf = _scene->queryBSDFEx(eye.surface, -light.omega(), eye.omega);

    float distSqInv = 1.0f / distance2(eye.position(), light.position());
    float eCosTheta = abs(dot(light.omega(), eye.gnormal()));
    float lCosTheta = abs(dot(light.omega(), light.normal()));

    float weightInv =
        bsdf.density() * lCosTheta * distSqInv / light.areaDensity() +
        1.0f +
        (eye.C * bsdf.density() + eye.c) * eCosTheta * distSqInv * light.omegaDensity();

    float weight = 1.0f / weightInv;

    return
        weight *
        light.radiance() *
        eye.throughput *
        bsdf.throughput() *
        eCosTheta *
        lCosTheta *
        distSqInv /
        (eye.density * light.areaDensity());
}

vec3 BPT::_connect(const EyeVertex& eye, const LightVertex& light)
{





    return vec3(0.0f);
}

vec3 BPT::_connect(
    RandomEngine& engine,
    const EyeVertex& eye,
    size_t size,
    const LightVertex* path)
{
    vec3 radiance = _connect(engine, eye);

    for (size_t i = 0; i < size; ++i)
        radiance += _connect(eye, path[i]);

    return radiance;
}

}
