#include <sstream>
#include <GLFW/glfw3.h>
#include <PathTracing.hpp>

namespace haste {

vec3 sampleLight(
    const Scene& scene,
    const vec3& position,
    const vec3& normal,
    const vec3& reflected,
    const mat3& worldToLight,
    const BSDF& bsdf)
{
    auto light = scene.lights.sample(position);

    if (light.radiance != vec3(0.0f)) {
        vec3 incident = light.position - position;
        float distance = length(incident);
        incident = normalize(incident);
        float sqDistanceInv = 1.f / (distance * distance);

        vec3 throughput = bsdf.eval(
            worldToLight * incident,
            worldToLight * reflected);

        float visible = scene.occluded(position, light.position);
        float geometry = max(0.f, dot(incident, normal)) * sqDistanceInv;

        return throughput * light.radiance * visible * geometry;
    }
    else {
        return vec3(0.0f);
    }
}

vec3 pathtrace(
    Ray ray,
    const Scene& scene)
{
    vec3 throughput = vec3(1.0f);
    vec3 accum = vec3(0.0f);
    bool specular = 0;
    int bounce = 0;

    while (true) {
        auto isect = scene.intersect(ray.origin, ray.direction);

        while (scene.isLight(isect)) {
            if (bounce == 0 || specular) {
                accum += throughput * scene.lights.eval(isect);
            }

            ray.origin = isect.position;
            isect = scene.intersect(ray.origin, ray.direction);
        }

        if (!isect.isPresent()) {
            break;
        }

        auto& bsdf = scene.queryBSDF(isect);

        SurfacePoint point = scene.querySurface(isect);

        vec3 normal = point.toWorldM[1];

        mat3 lightToWorld = point.toWorldM;
        mat3 worldToLight = transpose(lightToWorld);

        accum += throughput * sampleLight(
            scene,
            isect.position,
            normal,
            -ray.direction,
            worldToLight,
            bsdf);

        auto bsdfSample = bsdf.sample(
            lightToWorld,
            worldToLight,
            -ray.direction);

        throughput *= bsdfSample.throughput * dot(normal, bsdfSample.direction);

        ray.direction = bsdfSample.direction;
        ray.origin = isect.position;

        float prob = min(0.5f, length(throughput));

        if (prob < scene.sampler.sample()) {
            break;
        }
        else {
            throughput /= prob;
        }

        ++bounce;
    }

    return accum;
}

PathTracing::PathTracing() { }

void PathTracing::updateInteractive(double timeQuantum) {
    double startTime = glfwGetTime();
    size_t startRays = _scene->numRays();

    _progress = renderInteractive(_image, _width, *_camera, [&](Ray ray) -> vec3 {
        return pathtrace(ray, *_scene);
    });

    _renderTime += glfwGetTime() - startTime;
    _numRays += _scene->numRays() - startRays;
}

string PathTracing::stageName() const {
    std::stringstream stream;
    stream << "Tracing paths (" << numSamples() << " samples)";
    return stream.str();
}

double PathTracing::stageProgress() const {
    return _progress;
}

}
