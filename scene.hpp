#pragma once
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <glm/glm.hpp>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>


using namespace glm; 

struct intersect_t {
    size_t shape = 0;
    size_t face = 0;
    float depth = NAN;
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

namespace haste {

using std::vector;
using std::string;
using std::size_t;

struct Material {
	string name;
	vec3 diffuse;
};

struct Mesh {
	string name;
	unsigned geometryID;
	unsigned materialID;
};

struct Scene {
	RTCScene rtcScene;
	vector<Material> materials;
	vector<Mesh> meshes;
};

Scene loadScene(RTCDevice device, string path);

}
