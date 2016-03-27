#include <pathtrace.hpp>

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

        auto& bsdf = scene.queryMaterial(isect).bsdf;

        vec3 normal = normalize(scene.lerpNormal(isect));
        vec3 tangent = vec3(1, 1, 0); // dummy
        vec3 binormal = normalize(cross(tangent, normal));
        tangent = normalize(cross(normal, binormal));
        mat3 lightToWorld = mat3(binormal, normal, tangent);
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


    /*if (intersect.hit()) {
        if (scene.isMesh(intersect)) {
            auto light = scene.sampleLight();

            auto& material = scene.material(intersect);

            vec3 normal = scene.lerpNormal(intersect);
            vec3 tangent = vec3(1, 0, 0); // dummy

            vec3 incident = light.position - intersect.position;
            float distanceInv = 1.f / length(incident);
            incident *= distanceInv;
            vec3 reflected = -ray.direction;

            float V = scene.occluded(intersect.position, light.position);
            float G = max(0.f, dot(-incident, light.normal)) * max(0.f, dot(incident, normal)) * distanceInv;

            vec3 f = material.brdf(normal, tangent, incident, reflected);

            return f * light.power * V * G;
        }
        else {
            vec3 normal = scene.areaLights.lerpNormal(intersect);
            return scene.lightExitance(intersect) * dot(normal, -ray.direction);
        }
    }*/
    
    return accum;
}

size_t pathtraceInteractive(
    std::vector<vec4>& image,
    size_t pitch,
    const Camera& camera,
    const Scene& scene)
{
	return renderInteractive(image, pitch, camera, [&](Ray ray) -> vec3 {
        return pathtrace(ray, scene);
    });
}

}
