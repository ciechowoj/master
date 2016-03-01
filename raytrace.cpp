#include <cmath>
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

struct intersect_t {
    size_t shape = 0;
    size_t face = 0;
    float depth = NAN;
};

intersect_t trace(const obj::scene_t& scene, const ray_t& ray) {
    intersect_t result;
    float min_depth = INFINITY;

    for (size_t shape = 0; shape < scene.shapes.size(); ++shape) {
        const size_t num_faces = scene.shapes[shape].mesh.indices.size() / 3;
        const float* positions = scene.shapes[shape].mesh.positions.data();
        const unsigned* indices = scene.shapes[shape].mesh.indices.data();

        for (size_t face = 0; face < num_faces; ++face) {
            float depth = intersect(positions, indices + face * 3, ray);

            if (depth < min_depth) {
                min_depth = depth;
                result.depth = depth;
                result.shape = shape;
                result.face = face;
            }
        }
    }

    return result;
}

void raytrace(std::vector<vec3>& image, int width, int height, const camera_t& camera, const obj::scene_t& scene) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {

            ray_t ray;
            ray.pos = (camera.view * vec4(0.f, 0.f, 0.f, 1.f)).xyz();
            ray.dir = (camera.view * vec4(shoot(width, height, x, y, camera.fovy), 0.f)).xyz();



            if (!isnan(trace(scene, ray).depth)) {
                image[y * width + x] = vec3(1.f);
            }
            else {
                image[y * width + x] = vec3(1.f, 0.f, 0.f);
            }
        }
    }
}


