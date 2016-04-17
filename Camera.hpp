#pragma once
#include <functional>
#include <vector>
#include <Sample.hpp>

namespace haste {

using std::vector;
using std::function;
using namespace glm;

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct Camera {
    mat4 view;
    float fovy = pi<float>() / 3.f;

    mat4 proj(size_t width, size_t height) const;
};

struct ImageDesc {
    int x = 0, y = 0, w = 0, h = 0;
    size_t pitch = 0;
};

size_t render(
    RandomEngine& source,
    vector<vec4>& imageData,
    const ImageDesc& imageDesc,
    const Camera& camera,
    const function<vec3(RandomEngine& source, Ray ray)>& trace);

size_t render(
    RandomEngine& source,
    vector<vec4>& imageData,
    size_t pitch,
    const Camera& camera,
    const function<vec3(RandomEngine& source, Ray ray)>& trace);

double renderInteractive(
    vector<vec4>& imageData,
    size_t pitch,
    const Camera& camera,
    const function<vec3(RandomEngine& source, Ray ray)>& trace);

size_t renderGammaBoard(
    vector<vec4>& imageData,
    size_t pitch);

}
