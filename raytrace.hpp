#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <scene.hpp>
#include <camera.hpp>

using namespace glm;


static const float epsilon = 0.00001f;

inline bool is_zero(float a) {
    return -::epsilon < a && a < ::epsilon;
}


bool eq(float fa, float fb, int ulps = 2);
bool eq(const vec3& a, const vec3& b);

vec3 shoot(int width, int height, int x, int y, float fovy);

int raytrace(
    std::vector<vec4>& image, 
    size_t pitch,
    const haste::Camera& camera, 
    const haste::Scene& scene);







