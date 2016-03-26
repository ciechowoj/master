#pragma once
#include <glm>
#include <utility.hpp>

namespace haste {

using std::vector;
using std::string;
using namespace glm;

class Scene;
struct RayIsect;

struct LightSample {
    vec3 radiance;
    vec3 incident;
    vec3 position;
};

struct LightPhoton {
	vec3 position;
	vec3 direction;
	vec3 power;
};

struct SurfacePoint {
    vec3 position;
    mat3 toWorldM;
};

class AreaLights {
public:
    vector<string> names;
    vector<size_t> offsets;
    vector<int> indices;
    vector<vec3> exitances;
    vector<vec3> vertices;
    vector<mat3> toWorldMs;

    size_t numLights() const;
    size_t numFaces() const;
    float faceArea(size_t face) const;
    float facePower(size_t face) const;
    vec3 lerpPosition(size_t face, vec3 uvw) const;
    vec3 lerpNormal(size_t face, vec3 uvw) const;
    vec3 lerpNormal(const RayIsect& hit) const;

    float queryTotalPower() const;
    vec3 queryTotalPower3() const;
    float queryAreaLightPower(size_t id) const;
    vec3 queryAreaLightPower3(size_t id) const;
    float queryAreaLightArea(size_t id) const;
    size_t sampleLight() const;
    SurfacePoint sampleSurface(size_t id) const;
    mat3 queryTransform(size_t id) const;

    LightPhoton emit() const;

    LightSample sample(const vec3& position) const;
    vec3 eval(const RayIsect& isect) const;

public:
    mutable PiecewiseSampler lightSampler;
    mutable BarycentricSampler faceSampler;
    mutable HemisphereCosineSampler cosineSampler;
    mutable vector<float> lightWeights;

    void buildLightStructs() const;

    friend class Scene;
};

}
