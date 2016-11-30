#include <PT.hpp>
#include <Edge.hpp>

namespace haste {

PathTracing::PathTracing(size_t minSubpath, float roulette, size_t num_threads)
    : Technique(num_threads), _minSubpath(minSubpath), _roulette(roulette) { }

vec3 PathTracing::_traceEye(render_context_t& context, Ray ray) {
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

    eye[prv].surface = _scene->querySurface(isect);
    eye[prv].omega = -ray.direction;
    eye[prv].throughput = vec3(1.0f);
    eye[prv].specular = 0.0f;
    eye[prv].density = 1.0f;

    size_t path_size = 2;

    while (true) {
        radiance += _connect(context, eye[prv]);

        auto bsdf = _scene->sampleBSDF(*context.engine, eye[prv].surface, eye[prv].omega);

        while (true) {
            isect = _scene->intersect(isect.position(), bsdf.omega());

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
                / bsdf.density();

            eye[prv].specular = bsdf.specular();
            eye[itr].density = eye[prv].density * edge.fGeometry * bsdf.density();

            if (isect.isLight()) {
                radiance += _connect_light(eye[prv], eye[itr]);
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

vec3 PathTracing::_connect_light(const EyeVertex& eye, const EyeVertex& light) {
    auto lsdf = _scene->queryLSDF(light.surface, light.omega);
    float weightInv = eye.density * lsdf.areaDensity() / light.density + 1.0f;

    if (eye.specular == 1.0f)
        weightInv = 1.0f;

    vec3 result = lsdf.radiance() * light.throughput / weightInv;

    return result;
}

vec3 PathTracing::_connect(render_context_t& context, const EyeVertex& eye) {
    LightSampleEx light = _scene->sampleLight(*context.engine);
    vec3 omega = normalize(eye.surface.position() - light.position());

    if (dot(omega, light.normal()) < 0.0f) {
        return vec3(0.0f);
    }

    auto eyeBSDF = _scene->queryBSDF(eye.surface, -omega, eye.omega);

    auto edge = Edge(light, eye, omega);

    float weightInv = eyeBSDF.densityRev * edge.bGeometry / light.areaDensity() + 1.0f;

    vec3 result = _scene->occluded(eye.surface.position(), light.position())
        * light.radiance()
        / light.areaDensity()
        * eye.throughput
        * eyeBSDF.throughput
        * edge.bCosTheta
        * edge.fGeometry
        / weightInv;

    return result;
}

string PathTracing::name() const {
    return "Path Tracing";
}

}
