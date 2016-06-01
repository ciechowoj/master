#include <iostream>
#include <VertexMerging.hpp>

namespace haste {

VCM::VCM(size_t numPhotons, size_t numGather, float maxRadius)
    : _numPhotons(numPhotons)
    , _numGather(numGather)
    , _maxRadius(maxRadius) { }

void VCM::preprocess(
    const shared<const Scene>& scene,
    RandomEngine& engine,
    const function<void(string, float)>& progress,
    bool parallel)
{
    Technique::preprocess(scene, engine, progress, parallel);

    _scatter( engine);
}

void VCM::render(
    ImageView& view,
    RandomEngine& engine,
    size_t cameraId)
{
    auto trace = [&](RandomEngine& engine, Ray ray) -> vec3 {
        return _trace(engine, ray);
    };

    for_each_ray(view, engine, _scene->cameras(), cameraId, trace);
}

string VCM::name() const {
    return "Vertex Merging";
}

float VCM::_eta() const {
    return float(_numPhotons) * pi<float>() * _maxRadius * _maxRadius;
}

vec3 VCM::_gather(const EyeVertex* eye, RandomEngine& engine) {
    return vec3(0.0f);
}

vec3 VCM::_merge(const EyeVertex* eye, const LightVertex* light) {
    return vec3(0.0f);
}

vec3 VCM::_connect(
    RandomEngine& engine,
    const EyeVertex& eye,
    const LightVertex* lightPath,
    size_t lightSize)
{
    LightSample light = _scene->sampleLight(engine, eye.position());
    vec3 throughput = _scene->queryBSDF(eye.surface, -light.omega(), eye.omega);
    float cosTheta = dot(eye.normal(), -light.omega());
    float density = eye.density * light.density();
    vec3 radiance = eye.throughput * throughput * cosTheta * light.radiance() / density;

    for (size_t i = 0; i < lightSize; ++i) {
        radiance += _connect(eye, lightPath[i]);
    }

    return radiance;
}

vec3 VCM::_connect(const EyeVertex& eye, const LightVertex& light) {







    return vec3(0.0f);
}

size_t VCM::_trace(LightVertex* path, RandomEngine& engine) {
    size_t itr = 0;
    size_t prv = 0;

    LightSampleEx light = _scene->lights.sample(engine);

    RayIsect isect = _scene->intersectMesh(light.position(), light.omega());

    if (!isect.isPresent())
        return itr;

    float distSqInv = 1.0f / distance2(light.position(), isect.position());
    float fgeometry, bgeometry;

    path[itr].surface = _scene->querySurface(isect);
    path[itr].omega = -light.omega();

    fgeometry =
        dot(path[itr].omega, path[itr].surface.normal()) / distSqInv;

    bgeometry =
        dot(light.omega(), light.normal()) / distSqInv;

    path[itr].throughput = light.radiance() * fgeometry;
    path[itr].density = light.density() * fgeometry;
    path[itr].b = fgeometry * light.omegaDensity();
    path[itr].B = light.areaDensity() * bgeometry / path[itr].b;
    path[itr].C = 0.0f;

    prv = itr;
    ++itr;

    const float eta = _eta();
    float roulette = 0.95f;
    float uniform = sampleUniform1(engine).value();
    float fcosTheta, bcosTheta;

    while (uniform < roulette) {
        BSDFSample bsdf =
            _scene->sampleBSDF(engine, path[prv].surface, path[prv].omega);

        isect = _scene->intersectMesh(path[prv].position(), bsdf.omega());

        if (!isect.isPresent()) {
            break;
        }

        path[itr].surface = _scene->querySurface(isect);
        path[itr].omega = -bsdf.omega();

        distSqInv = 1.0f / distance2(isect.position(), path[itr].position());
        fcosTheta = abs(dot(path[itr].normal(), path[itr].omega));
        bcosTheta = abs(dot(path[prv].normal(), bsdf.omega()));
        fgeometry = fcosTheta * distSqInv;
        bgeometry = bcosTheta * distSqInv;

        path[itr].throughput =
            path[prv].throughput *
            bsdf.throughput() *
            fgeometry *
            bcosTheta;

        path[itr].density =
            path[prv].density *
            bsdf.density() *
            fgeometry *
            roulette;

        path[itr].b = fgeometry * bsdf.density();

        path[itr].B =
            (path[prv].B * bsdf.density() + path[prv].b) *
            bgeometry /
            path[itr].b;

        path[itr].C =
            (path[prv].C * bsdf.density() + eta) *
            bgeometry /
            path[itr].b;

        uniform = sampleUniform1(engine).value();
        prv = itr;
        ++itr;
    }

    return itr;
}

vec3 VCM::_trace(RandomEngine& engine, const Ray& ray) {
    LightVertex lightPath[1024];
    size_t lightSize = _trace(lightPath, engine);

    vec3 radiance = vec3(0.0f);

    EyeVertex vertices[2];
    EyeVertex* current = &vertices[0];
    EyeVertex* previous = &vertices[1];

    RayIsect isect = _scene->intersect(ray.origin, ray.direction);

    while (isect.isLight()) {
        radiance += _scene->queryRadiance(isect);
        isect = _scene->intersect(isect.position(), ray.direction);
    }

    if (!isect.isPresent()) {
        return radiance;
    }

    current->surface = _scene->querySurface(isect);
    current->omega = -ray.direction;
    current->throughput = vec3(1.0f);
    current->density = 1.0f;
    current->specular = 1.0f;
    current->d = 0;
    current->D = 0;

    radiance += _gather(current, engine);
    radiance += _connect(engine, *current, lightPath, lightSize);

    swap(current, previous);

    const float eta = _eta();
    float roulette = 1.0f;
    float uniform = sampleUniform1(engine).value();
    float size = 2.0f;

    while (uniform < roulette) {
        auto bsdf =
            _scene->sampleBSDF(engine, previous->surface, previous->omega);

        isect = _scene->intersect(previous->position(), bsdf.omega());

        float cosTheta = abs(dot(previous->surface.normal(), bsdf.omega()));

        while (isect.isLight()) {
            float specular = previous->specular * bsdf.specular();

            if (specular != 0.0f)
            {
                radiance +=
                    _scene->queryRadiance(isect) *
                    previous->throughput *
                    bsdf.throughput() *
                    cosTheta /
                    (previous->density * bsdf.density() * roulette);
            }
            else
            {
                radiance +=
                    _scene->queryRadiance(isect) *
                    previous->throughput *
                    bsdf.throughput() *
                    cosTheta /
                    (previous->density * bsdf.density() * roulette);
            }

            isect = _scene->intersect(isect.position(), bsdf.omega());
        }

        if (!isect.isPresent()) {
            return radiance;
        }

        current->surface = _scene->querySurface(isect);
        current->omega = -bsdf.omega();

        float distanceSqInv = 1.0f / distance2(isect.position(), previous->position());

        float fgeometry =
            abs(dot(current->surface.normal(), current->omega)) * distanceSqInv;

        float bgeometry = cosTheta * distanceSqInv;

        current->throughput =
            previous->throughput *
            bsdf.throughput() *
            fgeometry *
            cosTheta;

        current->density =
            previous->density *
            bsdf.density() *
            fgeometry *
            roulette;

        current->specular =
            previous->specular *
            bsdf.specular();

        current->d = fgeometry * bsdf.density();

        current->D =
            (previous->D * bsdf.density() + previous->d + eta) *
            bgeometry /
            current->d;

        radiance += _gather(current, engine);
        radiance += _connect(engine, *current, lightPath, lightSize);
        swap(current, previous);

        size += 1.0f;
        uniform = sampleUniform1(engine).value();
        roulette = size < 0.0f ? 1.0f : 0.5f;
    }

    return radiance;
}

void VCM::_scatter(RandomEngine& engine)
{
    vector<LightVertex> vertices;

    const float eta = _eta();

    for (size_t i = 0; i < _numPhotons; ++i) {
        LightSampleEx light = _scene->lights.sample(engine);
        RayIsect isect = _scene->intersectMesh(light.position(), light.omega());

        if (!isect.isMesh())
            continue;

        vec3 d = isect.position() - light.position();
        float vg = dot(isect.normal(), -light.omega()) /  dot(d, d);
        float cg = dot(light.normal(), light.omega()) / dot(d, d);

        LightVertex vertex;
        vertex.surface = _scene->querySurface(isect);
        vertex.omega = -light.omega();
        vertex.throughput = light.radiance() * vg;
        vertex.density = light.density() * vg;
        vertex.B = light.areaDensity() * cg / (vg * light.omegaDensity());
        vertex.b = vg * light.omegaDensity();
        vertex.C = 0.0f;

        float roulette = 0.95f;
        float uniform = sampleUniform1(engine).value();

        while (uniform < roulette)
        {
            vertices.push_back(vertex);
            LightVertex previous = vertex;

            auto sample = _scene->sampleBSDF(engine, previous.surface, previous.omega);
            isect = _scene->intersectMesh(previous.position(), sample.omega());

            if (!isect.isMesh())
                break;

            vec3 distance = isect.position() - previous.position();
            float fgeometry = dot(isect.normal(), -sample.omega());
            float bgeometry = dot(previous.normal(), sample.omega());
            float fdensity = sample.density() * roulette;
            float bdensity = fdensity;

            vertex.surface = _scene->querySurface(isect);
            vertex.omega = -sample.omega();
            vertex.throughput =
                previous.throughput *
                sample.throughput() *
                dot(vertex.normal(), sample.omega()) *
                fgeometry;

            vertex.density =
                previous.density *
                fgeometry *
                fdensity;

            vertex.B =
                (previous.B * bdensity + previous.b) *
                bgeometry / (fgeometry * fdensity);

            vertex.b = fgeometry * fdensity;

            vertex.C =
                (previous.C * bdensity + eta) *
                bgeometry / (fgeometry * fdensity);

            uniform = sampleUniform1(engine).value();
        }
    }

    std::cout << "size: " << vertices.size() << std::endl;

    _vertices = KDTree3D<LightVertex>(move(vertices));
}

}
