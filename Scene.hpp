#pragma once
#include <Prerequisites.hpp>
#include <Intersector.hpp>

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

class Scene : public Intersector {
public:
    Scene(
        Cameras&& cameras,
        Materials&& materials,
        vector<Mesh>&& meshes,
        AreaLights&& areaLights);

    Cameras _cameras;
    const vector<Mesh> meshes;
    AreaLights lights;
    const Materials materials;

    const Cameras& cameras() const { return _cameras; }

    void buildAccelStructs(RTCDevice device);

    const BSDF& queryBSDF(const RayIsect& isect) const;
    vec3 lightExitance(const RayIsect& hit) const;
    vec3 lerpNormal(const RayIsect& hit) const;

    vec3 queryRadiance(const RayIsect& isect) const;
    SurfacePoint querySurface(const RayIsect& isect) const;

    LightSample sampleLight(
        RandomEngine& engine,
        const vec3& position) const;

    LightSampleEx sampleLightEx(
        RandomEngine& engine,
        const vec3& position) const;

    vec3 queryRadiance(
        const RayIsect& isect,
        const vec3& omega) const;

    const LSDFQuery queryLSDF(
        const RayIsect& isect,
        const vec3& omega) const;

    const RayIsect intersect(
        const vec3& origin,
        const vec3& direction) const override;

    const RayIsect intersect(
        const Ray& ray) const;

    const float occluded(const vec3& origin,
        const vec3& target) const override;

    const RayIsect intersectLight(
        const vec3& origin,
        const vec3& direction) const override;

    const size_t numNormalRays() const;
    const size_t numShadowRays() const;
    const size_t numRays() const;

    mutable UniformSampler sampler;

    const LightSampleEx sampleLight(
        RandomEngine& engine) const;

    const BSDFSample sampleBSDF(
        RandomEngine& engine,
        const SurfacePoint& surface,
        const vec3& omega) const;

    const BSDFSample sampleAdjointBSDF(
        RandomEngine& engine,
        const SurfacePoint& surface,
        const vec3& omega) const;

    const BSDFQuery queryBSDF(
        const SurfacePoint& surface,
        const vec3& incident,
        const vec3& outgoing) const;

    const BSDFQuery queryAdjointBSDF(
        const SurfacePoint& surface,
        const vec3& incident,
        const vec3& outgoing) const;

    const vec3 sampleDirectLightAngle(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omegaR,
        const BSDF& bsdf) const;

    const vec3 sampleDirectLightArea(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omegaR,
        const BSDF& bsdf) const;

    const vec3 sampleDirectLightMixed(
        RandomEngine& engine,
        const SurfacePoint& surface,
        const vec3& omega,
        const BSDF& bsdf) const;

private:
    mutable std::atomic<size_t> _numIntersectRays;
    mutable std::atomic<size_t> _numOccludedRays;

    mutable RTCScene rtcScene;
};

}
