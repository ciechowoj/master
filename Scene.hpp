#pragma once
#include <vector>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <BSDF.hpp>
#include <Lights.hpp>
#include <Materials.hpp>
#include <PhotonMap.hpp>

namespace haste {

using namespace glm;

using std::vector;
using std::string;
using std::size_t;
using std::move;

struct RayIsect : public RTCRay {
    vec3 position;

    bool isPresent() const;

    vec3 gnormal() const { return normalize(vec3(-Ng[0], -Ng[1], -Ng[2])); }
};

struct Mesh {
    string name;
    unsigned materialID;
    vector<int> indices;
    vector<vec3> bitangents;
    vector<vec3> normals;
    vector<vec3> tangents;
    vector<vec3> vertices;
};

struct SurfacePoint {
    vec3 position;
    mat3 toWorldM;
    size_t materialID;
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
    const PhotonMap photons;
    const Materials materials;

    void buildAccelStructs(RTCDevice device);

    bool isMesh(const RayIsect& hit) const;
    bool isLight(const RayIsect& isect) const;

    const BSDF& queryBSDF(const RayIsect& hit) const;
    vec3 lightExitance(const RayIsect& hit) const;
    vec3 lerpNormal(const RayIsect& hit) const;

    SurfacePoint querySurface(const RayIsect& isect) const;

    RayIsect intersect(const vec3& origin, const vec3& direction) const;
    float occluded(const vec3& origin, const vec3& target) const;

    mutable UniformSampler sampler;
private:
    mutable RTCScene rtcScene;
};

inline bool RayIsect::isPresent() const {
    return geomID != RTC_INVALID_GEOMETRY_ID;
}

}
