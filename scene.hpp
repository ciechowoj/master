#pragma once
#include <vector>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <BSDF.hpp>

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

struct LightSample {
    vec3 radiance;
    vec3 incident;
    vec3 position;
};

class Scene;

class AreaLights {
public:
    vector<string> names;
    vector<size_t> offsets;
    vector<int> indices;
    vector<vec3> radiances;
    vector<vec3> vertices;
    vector<vec3> normals;

    size_t numLights() const;
    size_t numFaces() const;
    float faceArea(size_t face) const;
    float facePower(size_t face) const;
    vec3 lerpPosition(size_t face, vec3 uvw) const;
    vec3 lerpNormal(size_t face, vec3 uvw) const;
    vec3 lerpNormal(const RayIsect& hit) const;

    LightSample sample(const vec3& position) const;
    vec3 eval(const RayIsect& isect) const;

public:
    mutable PiecewiseSampler lightSampler;
    mutable BarycentricSampler faceSampler;
    mutable vector<float> lightWeights;

    void buildLightStructs() const;

    friend class Scene;
};


class Scene {
public:
    Scene(
        vector<Material>&& materials,
        vector<Mesh>&& meshes,
        AreaLights&& areaLights);

    const vector<Material> materials;
    const vector<Mesh> meshes;
    const AreaLights lights;

    void buildAccelStructs(RTCDevice device);

    bool isMesh(const RayIsect& hit) const;
    bool isLight(const RayIsect& isect) const;

    const Material& material(const RayIsect& hit) const;
    vec3 lightExitance(const RayIsect& hit) const;
    vec3 lerpNormal(const RayIsect& hit) const;

    RayIsect intersect(const vec3& origin, const vec3& direction) const;
    float occluded(const vec3& origin, const vec3& target) const;

private:
    mutable RTCScene rtcScene;
};

inline bool RayIsect::isPresent() const {
    return geomID != RTC_INVALID_GEOMETRY_ID;
}

}
