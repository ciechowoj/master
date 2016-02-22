#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <loader.hpp>

using namespace glm;

struct camera_t {
    camera_t(
        const vec3& position, 
        const vec3& direction, 
        float aspect, 
        float fovy = glm::pi<float>() / 3.f) 
        : position(position) 
        , direction(direction)
        , aspect(aspect) 
        , fovy(fovy) {
    }

    camera_t(
        const vec3& position, 
        const vec3& direction, 
        int width, 
        int height, 
        float fovy = glm::pi<float>() / 3.f)
        : camera_t(position, direction, float(width) / float(height), fovy) {
    }

    vec3 position;
    vec3 direction;
    float aspect;
    float fovy;
};

struct ray_t {
    vec3 pos, dir;

    ray_t()
        : pos(0.0f)
        , dir(0.0f, 0.0f, -1.0f) {
    }

    ray_t(float px, float py, float pz, float dx, float dy, float dz)
        : pos(px, py, pz)
        , dir(dx, dy, dz) { 
    }
};

static const float epsilon = 0.00001f;

inline bool is_zero(float a) {
    return -::epsilon < a && a < ::epsilon;
}

inline bool intersect(const vec3* triangle, const ray_t& ray) { 
    vec3 e1 = triangle[1] - triangle[0];
    vec3 e2 = triangle[2] - triangle[0];

    vec3 p = cross(ray.dir, e2);

    float det = dot(e1, p);

    if (is_zero(det)) {
        return false;
    }

    float inv_det = 1.f / det;
 
    vec3 t = ray.pos - triangle[0];

    float u = dot(t, p) * inv_det;

    if (u < 0.f || u > 1.f) {
        return false; 
    }
 
    vec3 q = cross(t, e1);

    float v = dot(ray.dir, q) * inv_det;

    if (v < 0.f || u + v > 1.f) {
        return false;
    }
 
    // float r = dot(e2, q) * inv_det;
 
    return true;
} 

vec3 shoot(int width, int height, int x, int y, float fovy);

void raytrace(std::vector<vec3>& image, int width, int height, const camera_t& camera, const scene_t& scene);









