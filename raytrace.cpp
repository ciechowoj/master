#include <cmath>
#include <GLFW/glfw3.h>
#include <tbb/tbb.h>
#include <raytrace.hpp>
#include <streamops.hpp>

using namespace std;
using namespace glm;

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

intersect_t trace(const haste::Scene& scene, const ray_t& ray) {
    RTCRay rtcRay;
    (*(vec3*)rtcRay.org) = ray.pos;
    (*(vec3*)rtcRay.dir) = ray.dir;
    rtcRay.tnear = 0.f;
    rtcRay.tfar = INFINITY;
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.mask = 0xFFFFFFFF;
    rtcRay.time = 0.f;
    rtcIntersect (scene.rtcScene, rtcRay);

    intersect_t result;
    result.shape = rtcRay.geomID;
    result.face = rtcRay.instID;

    if (rtcRay.geomID != RTC_INVALID_GEOMETRY_ID) {
        result.depth = rtcRay.tfar;
    }
    else {
        result.depth = NAN;
    }

    return result;
}

vec3 trace_color(const haste::Scene& scene, const ray_t& ray) {
    auto intersect = trace(scene, ray);

    if (!isnan(intersect.depth)) {
        int material_id = scene.meshes[intersect.shape].materialID;

        return scene.materials[material_id].diffuse;
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

                image[y * width + x] = trace_color(scene, ray);
            }
        });
    }

    return num_lines;
}


