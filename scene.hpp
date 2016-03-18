#pragma once
#include <vector>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include <utility.hpp>

namespace haste {

using namespace glm; 

using std::vector;
using std::string;
using std::size_t;
using std::move;

struct Material {
	string name;
    vec3 ambient;
	vec3 diffuse;
    vec3 emissive;

    BxDF brdf;
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

struct LightSample {
    vec3 power;
    vec3 position;
    vec3 normal;
};

class Scene {
public:
    Scene(
        vector<Material>&& materials,
        vector<Mesh>&& meshes,
        AreaLights&& areaLights);

    const vector<Material> materials;
    const vector<Mesh> meshes;
    const AreaLights areaLights;

    void buildAccelStructs(RTCDevice device) const;
    

    RTCRay intersect(const vec3& origin, const vec3& direction) const;
    bool occluded(const vec3& origin, const vec3& target) const;

    LightSample sampleLight() const;

private:
    mutable RTCScene rtcScene;
    mutable PiecewiseSampler lightSampler;
    mutable vector<float> lightWeights;
    mutable BarycentricSampler faceSampler;

    void buildLightStructs() const;

};

}
