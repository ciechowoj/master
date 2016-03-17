#pragma once
#include <functional>
#include <vector>
#include <glm>

namespace haste {

using std::vector;
using std::function;

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Camera {
    mat4 view;
    float fovy = glm::pi<float>() / 3.f;
};

struct ImageDesc {
    int x = 0, y = 0, w = 0, h = 0;
    size_t pitch = 0;
};

void render(
    vector<vec4>& imageData,
    const ImageDesc& imageDesc,
    const Camera& camera,
    const function<vec3(Ray ray)>& trace);

void render(
    vector<vec4>& imageData,
    size_t pitch,
    const Camera& camera,
    const function<vec3(Ray ray)>& trace);

void renderInteractive(
    vector<vec4>& imageData,
    size_t pitch,
    const Camera& camera,
    const function<vec3(Ray ray)>& trace);

}
