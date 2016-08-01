#pragma once
#include <Scene.hpp>

namespace haste {

shared<Scene> loadScene(string path);

struct Triangle {
    vec3 vertices[3];

    vec3 operator[](size_t i) const
    {
        return vertices[i];
    }

    vec3& operator[](size_t i)
    {
        return vertices[i];
    }

    float area() const
    {
        vec3 a = vertices[1] - vertices[0];
        vec3 b = vertices[2] - vertices[0];

        return length(cross(a, b)) * 0.5f;
    }

    vec3 interp(float u, float v) const
    {
        vec3 a = vertices[1] - vertices[0];
        vec3 b = vertices[2] - vertices[0];

        return a * u + b * v;
    }
};

vector<Triangle> loadTriangles(string path);

}
