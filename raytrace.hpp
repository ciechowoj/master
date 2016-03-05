#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <scene.hpp>

using namespace glm;

struct camera_t {
    mat4 view;
    float fovy = glm::pi<float>() / 3.f;
};

static const float epsilon = 0.00001f;

inline bool is_zero(float a) {
    return -::epsilon < a && a < ::epsilon;
}

float intersect(const vec3* triangle, const ray_t& ray);

bool eq(float fa, float fb, int ulps = 2);
bool eq(const vec3& a, const vec3& b);

vec3 shoot(int width, int height, int x, int y, float fovy);

int raytrace(
    std::vector<vec3>& image, 
    int width, 
    int height, 
    const camera_t& camera, 
    const haste::Scene& scene, 
    float budget,
    int& line);







