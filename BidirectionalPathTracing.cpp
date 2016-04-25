#include <iostream>
#include <sstream>
#include <GLFW/glfw3.h>
#include <BidirectionalPathTracing.hpp>

namespace haste {

BidirectionalPathTracing::BidirectionalPathTracing() { }

void BidirectionalPathTracing::render(
   ImageView& view,
   RandomEngine& engine,
   size_t cameraId)
{
    auto trace = [&](RandomEngine& engine, Ray ray) -> vec3 {
        return this->trace(engine, ray);
    };

    for_each_ray(view, engine, _scene->cameras(), cameraId, trace);
}

vec3 BidirectionalPathTracing::trace(RandomEngine& engine, Ray ray) {
    Vertex lightSubpath[_maxSubpathLength];
    Vertex eyeSubpath[_maxSubpathLength];

    size_t lightSubpathSize = 0;
    size_t eyeSubpathSize = 0;
    vec3 color = vec3(0.0f);

    lightSubpathSize = traceLightSubpath(engine, lightSubpath);
    std::tie(eyeSubpathSize, color) = traceEyeSubpath(engine, eyeSubpath, ray);


    for (size_t i = 1; i < eyeSubpathSize; ++i) {
        auto& eye = eyeSubpath[i];
        auto& light = lightSubpath[0];
        auto omega = normalize(light.position() - eye.position());

        float geometry =
            dot(light.normal(), -omega) * dot(eye.normal(), omega)
            / distance2(light.position(), eye.position());

        vec3 throughput = eye.bsdf()->query(eye.point(), omega, eye.normal(), eye.gnormal());
        float visibility = _scene->occluded(light.position(), eye.position());
        float density = light.density() * eye.density();

        color +=
            (dot(light.normal(), -omega) > 0.0f ? 1.0f : 0.0f) *
            light.throughput() *
            eye.throughput() *
            visibility *
            throughput *
            geometry /
            density;

        for (size_t j = 1; j < lightSubpathSize; ++j) {
            auto& light = lightSubpath[j];
            auto omega = normalize(light.position() - eye.position());

            vec3 throughput =
                light.bsdf()->query(light.point(), light.omega(), -omega, light.gnormal()) *
                eye.bsdf()->query(eye.point(), eye.omega(), omega, eye.gnormal());

            float geometry =
                dot(light.normal(), -omega) * dot(eye.normal(), omega)
                / distance2(light.position(), eye.position());

            float visibility = _scene->occluded(light.position(), eye.position());
            float density = light.density() * eye.density();

            color +=
                light.throughput() *
                eye.throughput() *
                visibility *
                throughput *
                geometry /
                (density * float(i + j));
        }
    }

    return color; // / float(lightSubpathSize);
}

size_t BidirectionalPathTracing::traceLightSubpath(RandomEngine& engine, Vertex* subpath) {
    auto lightSample = _scene->sampleLight(engine);

    vec3 omega = lightSample.omega();
    subpath[0]._point._position = lightSample.position();
    subpath[0]._point.toWorldM[1] = lightSample.normal();
    subpath[0]._throughput = lightSample.radiance();
    subpath[0]._density = lightSample.areaDensity();

    auto isect = _scene->intersect(subpath[0].position(), omega);

    while (isect.isLight()) {
        isect = _scene->intersect(isect.position(), omega);
    }

    if (isect.isPresent()) {
        subpath[1]._point = _scene->querySurface(isect);
        subpath[1]._omega = -omega;
        subpath[1]._throughput =
            lightSample.radiance() *
            dot(subpath[0].normal(), omega) *
            dot(subpath[1].normal(), -omega) /
            distance2(subpath[0].position(), subpath[1].position());

        subpath[1]._density =
            lightSample.density() *
            dot(subpath[1].normal(), -omega) /
            distance2(subpath[0].position(), subpath[1].position());

        subpath[1]._bsdf = &_scene->queryBSDF(isect);

        return traceSubpath(engine, subpath, omega);
    }
    else {
        return 1;
    }
}

pair<size_t, vec3> BidirectionalPathTracing::traceEyeSubpath(RandomEngine& engine, Vertex* subpath, Ray ray) {
    subpath[0]._point._position = ray.origin;
    subpath[0]._throughput = vec3(1.0f);
    subpath[0]._density = 1;

    vec3 extra = vec3(0.0f);

    auto isect = _scene->intersect(ray.origin, ray.direction);

    while (isect.isLight()) {
        extra += _scene->queryRadiance(isect);
        isect = _scene->intersect(isect.position(), ray.direction);
    }

    if (isect.isPresent()) {
        subpath[1]._point = _scene->querySurface(isect);
        subpath[1]._omega = -ray.direction;
        subpath[1]._throughput = subpath[0]._throughput;
        subpath[1]._density = subpath[0]._density;
        subpath[1]._bsdf = &_scene->queryBSDF(isect);

        return std::make_pair(traceSubpath(engine, subpath, ray.direction), extra);
    }
    else {
        return std::make_pair(size_t(1), extra);
    }
}

size_t BidirectionalPathTracing::traceSubpath(
    RandomEngine& engine,
    Vertex* subpath,
    vec3 omega)
{
    static const float russianRouletteInv = _russianRouletteInv1K / 1000.0f;
    static const float russianRoulette = 1.0f / float(russianRouletteInv);

    size_t size = 2;

    float roulette = min(russianRoulette, dot(subpath[size - 1]._throughput, vec3(1.0f)));
    float uniform = sampleUniform1(engine).value();

    while (size < _maxSubpathLength && uniform < roulette) {
        auto bsdf = subpath[size - 1].bsdf();
        auto sample = bsdf->sample(engine, subpath[size - 1].point(), -omega);

        if (!sample.zero()) {
            RayIsect isect = _scene->intersect(subpath[size - 1].position(), sample.omega());

            while (isect.isLight()) {
                isect = _scene->intersect(isect.position(), sample.omega());
            }

            if (isect.isPresent()) {
                float geometry = dot(-sample.omega(), isect.normal()) /
                    distance2(subpath[size - 1].position(), isect.position());

                subpath[size]._point = _scene->querySurface(isect);

                subpath[size]._omega = -sample.omega();

                subpath[size]._throughput =
                    subpath[size - 1].throughput() *
                    sample.throughput() *
                    dot(sample.omega(), subpath[size - 1].normal()) *
                    geometry;

                subpath[size]._density =
                    subpath[size - 1].density() *
                    sample.density() *
                    geometry *
                    roulette;

                subpath[size]._bsdf = &_scene->queryBSDF(isect);

                size += sample.zero() ? 0 : 1;

                omega = sample.omega();
                roulette = min(russianRoulette, dot(subpath[size]._throughput, vec3(1.0f)));
                uniform = sampleUniform1(engine).value();
            }
            else {
                break;
            }
        }
        else {
            break;
        }
    }

    return size;
}

string BidirectionalPathTracing::name() const {
    return "Bidirectional Path Tracing";
}

}
