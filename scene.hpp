#pragma once
#include <iostream>
#include <stdexcept>
#include <vector>
#include <random>
#include <cmath>
#include <glm/glm.hpp>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <utility.hpp>

using namespace glm; 

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
    vec3 ambient;
	vec3 diffuse;
    vec3 emissive;
};

struct Mesh {
	string name;
	unsigned geometryID;
	unsigned materialID;
    vector<int> indices;
    vector<vec3> normals;
    vector<vec3> vertices;
};

struct AreaLights {
    vector<string> names;
    vector<size_t> offsets;
    vector<int> indices;
    vector<vec3> wattages;
    vector<vec3> vertices;
    vector<vec3> normals;

    size_t numLights() const;
    size_t numFaces() const;
    float faceArea(size_t face) const;
    float facePower(size_t face) const;
    vec3 lerpPosition(size_t face, vec3 uvw) const;
    vec3 lerpNormal(size_t face, vec3 uvw) const;
};

struct Scene {
    vector<Material> materials;
    vector<Mesh> meshes;
    AreaLights areaLights;
};

struct LightSample {
    vec3 power;
    vec3 position;
    vec3 normal;
};

struct LightCache {
    const AreaLights* lights;
    PiecewiseSampler lightsSampler;
    BarycentricSampler faceSampler;
    vector<float> weights;

    LightSample sampleLight();
};

struct SceneCache {
    const Scene* scene = nullptr;
    RTCScene rtcScene = nullptr;
    LightCache lightCache;
};

void updateRTCScene(RTCScene& rtcScene, RTCDevice device, const Scene& scene);
void updateCache(SceneCache& cache, RTCDevice device, const Scene& scene);

}
