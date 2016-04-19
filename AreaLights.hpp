#pragma once
#include <utility.hpp>
#include <RayIsect.hpp>
#include <SurfacePoint.hpp>

namespace haste {

using std::vector;
using std::string;
using namespace glm;

struct Photon {
    vec3 position;
    vec3 direction;
    vec3 power;

    float operator[](size_t index) const {
        return position[index];
    }

    float& operator[](size_t index) {
        return position[index];
    }
};

struct LightSample {
    vec3 _position;
    vec3 _radiance;
    vec3 _omega;
    float _density;

    const vec3& position() const { return _position; }
    const vec3& radiance() const { return _radiance; }
    const vec3& omega() const { return _omega; }
    const float density() const { return _density; }
    const float densityInv() const { return 1.0f / _density; }
};

struct LightPoint {
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

    float queryTotalPower() const;
    vec3 queryTotalPower3() const;
    float queryAreaLightPower(size_t id) const;
    vec3 queryAreaLightPower3(size_t id) const;
    float queryAreaLightArea(size_t id) const;

    const vec3& faceNormal(size_t faceId) const;
    const vec3 faceRadiance(size_t faceId) const;
    const vec3 faceRadiance(const RayIsect& isect) const;

    size_t sampleFace() const;



    LightPoint sampleSurface(size_t id) const;


    Photon emit() const;


    LightSample sample(
        RandomEngine& engine,
        const vec3& position) const;
public:
    mutable RandomEngine source;
    mutable PiecewiseSampler lightSampler;
    mutable BarycentricSampler faceSampler;
    mutable vector<float> lightWeights;

    void buildLightStructs() const;
};

}
