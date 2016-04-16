#pragma once
#include <memory>
#include <vector>
#include <atomic>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <BSDF.hpp>
#include <Lights.hpp>
#include <Materials.hpp>

namespace haste {

using namespace glm;

template <class T> using shared = std::shared_ptr<T>;
using std::make_shared;

using std::vector;
using std::string;
using std::size_t;
using std::move;

struct Ray;

struct RayIsect : public RTCRay {
private:
    using cpvec3 = const vec3*;

public:
    bool isPresent() const;
    bool isLight() const;
    bool isGeometry() const;

    vec3 gnormal() const {
        return normalize(vec3(-Ng[0], -Ng[1], -Ng[2]));
    }

    vec3 position() const {
        return *cpvec3(org) + *cpvec3(dir) * tfar;
    }

    const vec3 incident() const {
        return -normalize(*cpvec3(dir));
    }
};

struct Mesh {
    string name;
    unsigned materialID;
    vector<int> indices;
    vector<vec3> vertices;
    vector<vec3> normals;
    vector<vec3> tangents;
    vector<vec3> bitangents;
};

struct SurfacePoint {
    vec3 position;
    mat3 toWorldM;
    size_t materialID;

    const vec3& normal() const {
        return toWorldM[1];
    }

    vec3 toWorld(const vec3& v) const {
        return toWorldM * v;
    }
};

struct LightSample2 {
    vec3 radiance;
    vec3 position;
    vec3 incident;
    float pdf;
};

class Scene;

class Scene {
public:
    Scene(
        Materials&& materials,
        vector<Mesh>&& meshes,
        Lights&& areaLights);

    const vector<Mesh> meshes;
    const Lights lights;
    const Materials materials;

    void buildAccelStructs(RTCDevice device);

    bool isMesh(const RayIsect& hit) const;
    bool isLight(const RayIsect& isect) const;

    const BSDF& queryBSDF(const RayIsect& hit) const;
    vec3 lightExitance(const RayIsect& hit) const;
    vec3 lerpNormal(const RayIsect& hit) const;

    vec3 queryRadiance(const RayIsect& isect) const;
    SurfacePoint querySurface(const RayIsect& isect) const;

    LightSample2 sampleLight() const;


    RayIsect intersect(const vec3& origin, const vec3& direction) const;
    RayIsect intersect(const Ray& ray) const;
    float occluded(const vec3& origin, const vec3& target) const;

    size_t numIntersectRays() const;
    size_t numOccludedRays() const;
    size_t numRays() const;

    mutable UniformSampler sampler;
private:
    mutable std::atomic<size_t> _numIntersectRays;
    mutable std::atomic<size_t> _numOccludedRays;

    mutable RTCScene rtcScene;
};

inline bool RayIsect::isPresent() const {
    return geomID != RTC_INVALID_GEOMETRY_ID;
}

}
