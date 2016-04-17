#include <sstream>
#include <GLFW/glfw3.h>
#include <PathTracing.hpp>

namespace haste {

/* vec3 sampleLight(
    const Scene& scene,
    const vec3& position,
    const vec3& normal,
    const vec3& reflected,
    const mat3& worldToLight,
    const BSDF& bsdf)
{
    auto light = scene.lights.sample(position);

    if (light.radiance() != vec3(0.0f)) {
        vec3 incident = light.position() - position;
        float distance = length(incident);
        incident = normalize(incident);
        float sqDistanceInv = 1.f / (distance * distance);

        vec3 throughput = bsdf.query(
            worldToLight * incident,
            worldToLight * reflected);

        float visible = scene.occluded(position, light.position());
        float geometry = max(0.f, dot(incident, normal)) * sqDistanceInv;

        return throughput * light.radiance() * visible * geometry;
    }
    else {
        return vec3(0.0f);
    }
}*/

vec3 pathtrace(
    RandomEngine& engine,
    Ray ray,
    const Scene& scene)
{
    const Scene* _scene = &scene;

    vec3 throughput = vec3(1.0f);
    vec3 radiance = vec3(0.0f);
    bool specular = 0;
    int bounce = 0;

    while (true) {
        auto isect = scene.intersect(ray.origin, ray.direction);

        while (scene.isLight(isect)) {
            if (bounce == 0 || specular) {
                radiance += throughput * scene.lights.eval(isect);
            }

            ray.origin = isect.position();
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

        auto lightSample = _scene->sampleLight(engine, point.position);
        auto localThroughput = bsdf.query(point, lightSample.omega(), -ray.direction);

        auto cosineTheta = dot(normal, lightSample.omega());
        if (cosineTheta > 0.0f) {
            radiance +=
                throughput *
                localThroughput *
                lightSample.radiance() *
                cosineTheta *
                lightSample.densityInv();
        }

        auto bsdfSample = bsdf.sample(
            engine,
            point,
            -ray.direction);

        throughput *= bsdfSample.throughput() * dot(normal, bsdfSample.omega()) * bsdfSample.densityInv();

        ray.direction = bsdfSample.omega();
        ray.origin = isect.position();

        float prob = min(0.5f, length(throughput));

        if (prob < scene.sampler.sample()) {
            break;
        }
        else {
            throughput /= prob;
        }

        ++bounce;
    }

    return radiance;
}

PathTracing::PathTracing() { }

void PathTracing::updateInteractive(
    size_t width,
    size_t height,
    vec4* image,
    double timeQuantum) {
    double startTime = glfwGetTime();
    size_t startRays = _scene->numRays();

    _progress = renderInteractive(width, height, image, *_camera, [&](RandomEngine& source, Ray ray) -> vec3 {
        return pathtrace(source, ray, *_scene);
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
