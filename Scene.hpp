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
    vector<mat3> tangents;
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
    const BSDF& queryBSDF(const SurfacePoint& surface) const;

    vec3 queryRadiance(const RayIsect& isect) const;

    SurfacePoint querySurface(const RayIsect& isect) const;

    vec3 queryRadiance(
        const RayIsect& isect,
        const vec3& omega) const;

    vec3 queryRadiance(
        const SurfacePoint& surface,
        const vec3& direction) const;

    const LSDFQuery queryLSDF(
        const RayIsect& isect,
        const vec3& omega) const;

    const LSDFQuery queryLSDF(
        const SurfacePoint& surface,
        const vec3& omega) const;

    using Intersector::intersect;

    float occluded(const SurfacePoint& origin,
        const SurfacePoint& target) const override;

    virtual SurfacePoint intersect(
        const SurfacePoint& surface,
        vec3 direction,
        float tfar) const override;

    const size_t numNormalRays() const;
    const size_t numShadowRays() const;
    const size_t numRays() const;

    mutable UniformSampler sampler;

    const LightSample sampleLight(
        RandomEngine& engine) const;

    const BSDFSample sampleBSDF(
        RandomEngine& engine,
        const SurfacePoint& surface,
        const vec3& omega) const;

    const BSDFQuery queryBSDF(
        const SurfacePoint& surface,
        const vec3& incident,
        const vec3& outgoing) const;

    bounding_sphere_t bounding_sphere() const;

private:
    int32_t _material_id_to_light_id(int32_t) const;
    int32_t _light_id_to_material_id(int32_t) const;

    bounding_sphere_t _bounding_sphere; // = bounding_sphere_t(0.0f);
    bounding_sphere_t _compute_bounding_sphere() const;

    mutable std::atomic<size_t> _numIntersectRays;
    mutable std::atomic<size_t> _numOccludedRays;

    mutable RTCScene rtcScene;
};

}
