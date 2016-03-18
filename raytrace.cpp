#include <cmath>
#include <GLFW/glfw3.h>
#include <tbb/tbb.h>
#include <raytrace.hpp>
#include <streamops.hpp>

using namespace std;
using namespace glm;
using namespace haste;

vec3 sampleLights(
    const Scene& scene, 
    const Material& material,
    const vec3& position, 
    const vec3& N, 
    const vec3& camera) 
{
    auto result = vec3(0.0f);

    /*for (auto&& light : scene.lights) {
        if (!occluded(scene, position, light.area.position)) {
            float D = length(light.area.position - position);
            vec3 L = normalize(light.area.position - position);
            result += dot(L, N) * material.diffuse * light.emissive / (D * D);
        }
    }*/

    return result;
}

vec3 trace_color(
    const haste::Scene& scene,
    const Ray& ray) {
    auto result = scene.intersect(ray.origin, ray.direction);

    if (result.geomID != RTC_INVALID_GEOMETRY_ID) {
        const Mesh* mesh = scene.meshes.data() + result.geomID;

        if (result.geomID < scene.meshes.size()) {

            int material_id = scene.meshes[result.geomID].materialID;

            float w = 1.f - result.u - result.v;
            vec3 N = w * mesh->normals[mesh->indices[result.primID * 3 + 0]] +
                     result.u * mesh->normals[mesh->indices[result.primID * 3 + 1]] +
                     result.v * mesh->normals[mesh->indices[result.primID * 3 + 2]];

            N = normalize(N);

            vec3 P = ray.origin + normalize(ray.direction) * result.tfar;

            return clamp(N * 0.5f + 0.5f, 0.0f, 1.0f);
        }
        else {
            return vec3(1);
        }
    }
    else {
        return vec3(0.f);
    }
}

int raytrace(
    std::vector<vec4>& image, 
    int width, 
    int height, 
    const haste::Camera& camera, 
    const haste::Scene& scene)
{
    return renderInteractive(image, width, camera, [&](Ray ray) -> vec3 {
        return trace_color(scene, ray);
    });
}


