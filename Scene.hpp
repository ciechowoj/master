#pragma once
#include <Prerequisites.hpp>
#include <RayIsect.hpp>

#include <BSDF.hpp>
#include <AreaLights.hpp>
#include <Materials.hpp>
#include <Cameras.hpp>

#include <SurfacePoint.hpp>

namespace haste {

using namespace glm;

template <class T> using shared = std::shared_ptr<T>;
using std::make_shared;

using std::vector;
using std::string;
using std::size_t;
using std::move;

struct Ray;

struct Mesh {
    string name;
    unsigned materialID;
    vector<int> indices;
    vector<vec3> vertices;
    vector<vec3> normals;
    vector<vec3> tangents;
    vector<vec3> bitangents;
};

struct DirectLightSample {
    vec3 _radiance;
    float _densityInv;

    const vec3& radiance() const { return _radiance; }
    const float density() const { return 1.0f / _densityInv; }
    const float densityInv() const { return _densityInv; }
};

class Scene {
public:
    Scene(
        Cameras&& cameras,
        Materials&& materials,
        vector<Mesh>&& meshes,
        AreaLights&& areaLights);

    Cameras _cameras;
    const vector<Mesh> meshes;
    const AreaLights lights;
    const Materials materials;

    void buildAccelStructs(RTCDevice device);

    const BSDF& queryBSDF(const RayIsect& hit) const;
    vec3 lightExitance(const RayIsect& hit) const;
    vec3 lerpNormal(const RayIsect& hit) const;

    vec3 queryRadiance(const RayIsect& isect) const;
    SurfacePoint querySurface(const RayIsect& isect) const;

    LightSample sampleLight(
        RandomEngine& engine,
        const vec3& position) const;

    RayIsect intersect(const vec3& origin, const vec3& direction) const;
    RayIsect intersect(const Ray& ray) const;
    float occluded(const vec3& origin, const vec3& target) const;

    size_t numIntersectRays() const;
    size_t numOccludedRays() const;
    size_t numRays() const;

    mutable UniformSampler sampler;



    const DirectLightSample sampleDirectLightAngle(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omegaR,
        const BSDF& bsdf) const;

    const DirectLightSample sampleDirectLightArea(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omegaR,
        const BSDF& bsdf) const;

    const DirectLightSample sampleDirectLightMixed(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omegaR,
        const BSDF& bsdf) const;



private:
    mutable std::atomic<size_t> _numIntersectRays;
    mutable std::atomic<size_t> _numOccludedRays;

    mutable RTCScene rtcScene;
};

}
