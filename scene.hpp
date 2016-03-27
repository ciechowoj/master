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

struct Material {
	string name;
    vec3 ambient;
	vec3 diffuse;
    vec3 emissive;

    BSDF bsdf;
};

struct Mesh {
	string name;
	unsigned geometryID;
	unsigned materialID;
    vector<int> indices;
    vector<vec3> normals;
    vector<vec3> vertices;
};

struct SurfacePoint {
    vec3 position;
    mat3 toWorldM;
};

class Scene;

class Scene {
public:
    Scene(
        vector<Material>&& materials,
        vector<Mesh>&& meshes,
        AreaLights&& areaLights);

    const vector<Material> materials;
    const vector<Mesh> meshes;
    const AreaLights lights;
    // const Materials materials;

    void buildAccelStructs(RTCDevice device);

    bool isMesh(const RayIsect& hit) const;
    bool isLight(const RayIsect& isect) const;

    const Material& queryMaterial(const RayIsect& hit) const;
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
