#include <cmath>
#include <GLFW/glfw3.h>
#include <tbb/tbb.h>
#include <raytrace.hpp>
#include <streamops.hpp>

using namespace std;
using namespace glm;
using namespace haste;

float intersect(const float* tris, const unsigned* indices, const ray_t& ray) {
    vec3 v0 = *(vec3*)(tris + indices[0] * 3);
    vec3 e1 = *(vec3*)(tris + indices[1] * 3) - v0;
    vec3 e2 = *(vec3*)(tris + indices[2] * 3) - v0;

    vec3 p = cross(ray.dir, e2);

    float det = dot(e1, p);

    if (is_zero(det)) {
        return NAN;
    }

    float inv_det = 1.f / det;
 
    vec3 t = ray.pos - v0;

    float u = dot(t, p) * inv_det;

    if (u < 0.f || u > 1.f) {
        return NAN;
    }
 
    vec3 q = cross(t, e1);

    float v = dot(ray.dir, q) * inv_det;

    if (v < 0.f || u + v > 1.f) {
        return NAN;
    }

    return dot(e2, q) * inv_det;
}

float intersect(const vec3* triangle, const ray_t& ray) { 
    unsigned indices[] = { 0, 1, 2 };
    return intersect((const float*)triangle, indices, ray);
}

bool eq(float fa, float fb, int ulps) {
    int a = *(int*)&fa;
    
    if (a < 0) {
        a = 0x80000000 - a;
    }

    int b = *(int*)&fb;

    if (b < 0) {
        b = 0x80000000 - b;
    }

    return abs(a - b) <= ulps;
}

bool eq(const vec3& a, const vec3& b) {
	return eq(a.x, b.x) && eq(a.y, b.y) && eq(a.z, b.z);
}

vec3 shoot(int width, int height, int x, int y, float fovy) {
    float znear = 1.f / tan(fovy * 0.5f);
    float aspect = float(width) / float(height);
    float fx = ((float(x) + 0.5f) / float(width) * 2.f - 1.f) * aspect;
    float fy = (float(y) + 0.5f) / float(height) * 2.f - 1.f;
    return normalize(vec3(fx, fy, -znear));
}

RTCRay intersect(const Cache& cache, const vec3& position, const vec3& direction) {
    RTCRay rtcRay;
    (*(vec3*)rtcRay.org) = position;
    (*(vec3*)rtcRay.dir) = direction;
    rtcRay.tnear = 0.f;
    rtcRay.tfar = INFINITY;
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.mask = 0xFFFFFFFF;
    rtcRay.time = 0.f;
    rtcIntersect(cache.rtcScene, rtcRay);
    return rtcRay;   
}

bool occluded(const Cache& cache, const vec3& source, const vec3& target) {
    vec3 direction = target - source;

    RTCRay rtcRay;
    (*(vec3*)rtcRay.org) = source;
    (*(vec3*)rtcRay.dir) = direction;
    rtcRay.tnear = 0.01f;
    rtcRay.tfar = 0.9f;
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.mask = 0xFFFFFFFF;
    rtcRay.time = 0.f;
    rtcOccluded(cache.rtcScene, rtcRay);
    return rtcRay.geomID == 0;
}

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
    const haste::Cache& cache,
    const ray_t& ray) {
    auto result = intersect(cache, ray.pos, ray.dir);

    if (result.geomID != RTC_INVALID_GEOMETRY_ID) {
        const Mesh* mesh = scene.meshes.data() + result.geomID;

        if (result.geomID < scene.meshes.size()) {

            int material_id = scene.meshes[result.geomID].materialID;

            float w = 1.f - result.u - result.v;
            vec3 N = w * mesh->normals[mesh->indices[result.primID * 3 + 0]] +
                     result.u * mesh->normals[mesh->indices[result.primID * 3 + 1]] +
                     result.v * mesh->normals[mesh->indices[result.primID * 3 + 2]];

            N = normalize(N);

            vec3 P = ray.pos + normalize(ray.dir) * result.tfar;

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
    std::vector<vec3>& image, 
    int width, 
    int height, 
    const camera_t& camera, 
    const haste::Scene& scene,
    const haste::Cache& cache,
    float budget, 
    int& line)
{
    double start = glfwGetTime();
    int num_lines = 0;

    while (glfwGetTime() < start + budget) {
        line = (line + 1) % height;
        ++num_lines;
        int y = line;
        parallel_for(
            tbb::blocked_range<int>(0, width), 
            [&](const tbb::blocked_range<int>& range) {
            for (int x = range.begin(); x < range.end(); ++x) {
                ray_t ray;
                ray.pos = (camera.view * vec4(0.f, 0.f, 0.f, 1.f)).xyz();
                ray.dir = (camera.view * vec4(shoot(width, height, x, y, camera.fovy), 0.f)).xyz();

                image[y * width + x] = trace_color(scene, cache, ray);
            }
        });
    }

    return num_lines;
}


