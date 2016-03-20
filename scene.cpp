#include <scene.hpp>
#include <streamops.hpp>
#include <cstring>

namespace haste {

size_t AreaLights::numLights() const {
    return names.size();
}

size_t AreaLights::numFaces() const {
    return indices.size() / 3;
}

float AreaLights::faceArea(size_t face) const {
    vec3 u = vertices[indices[face * 3 + 1]] - vertices[indices[face * 3 + 0]];
    vec3 v = vertices[indices[face * 3 + 2]] - vertices[indices[face * 3 + 0]];
    return length(cross(u, v)) * 0.5f;
}

float AreaLights::facePower(size_t face) const {
    return length(radiances[face]) * faceArea(face) * pi<float>();
}

vec3 AreaLights::lerpPosition(size_t face, vec3 uvw) const {
    return vertices[indices[face * 3 + 0]] * uvw.z
        + vertices[indices[face * 3 + 1]] * uvw.x
        + vertices[indices[face * 3 + 2]] * uvw.y;
}

vec3 AreaLights::lerpNormal(size_t face, vec3 uvw) const {
    return normals[indices[face * 3 + 0]] * uvw.z
        + normals[indices[face * 3 + 1]] * uvw.x
        + normals[indices[face * 3 + 2]] * uvw.y;
}

vec3 AreaLights::lerpNormal(const RayIsect& hit) const {
    float w = 1.0f - hit.u - hit.v;

    return normals[indices[hit.primID * 3 + 0]] * w
        + normals[indices[hit.primID * 3 + 1]] * hit.u
        + normals[indices[hit.primID * 3 + 2]] * hit.v;
}

LightSample AreaLights::sample(const vec3& position) const {
    size_t face = size_t(lightSampler.sample() * numFaces());
    vec3 uvw = faceSampler.sample();

    vec3 normal = lerpNormal(face, uvw);
    vec3 radiance = radiances[face] * lightWeights[face];

    LightSample sample;
    sample.position = lerpPosition(face, uvw);
    sample.incident = normalize(sample.position - position);
    sample.radiance = dot(normal, -sample.incident) < 0.0f ? vec3(0.0f) : radiance;

    return sample;
}

vec3 AreaLights::eval(const RayIsect& isect) const {
    return radiances[isect.primID];
}

void AreaLights::buildLightStructs() const {
    size_t numFaces = this->numFaces();
    lightWeights.resize(numFaces);

    float total = 0.f;
    for (size_t i = 0; i < numFaces; ++i) {
        lightWeights[i] = faceArea(i) / facePower(i);
        total += lightWeights[i];
    }

    float totalInv = 1.0f / total;

    for (size_t i = 0; i < numFaces; ++i) {
        lightWeights[i] *= totalInv;
    }

    lightSampler = PiecewiseSampler(
        lightWeights.data(), 
        lightWeights.data() + lightWeights.size());

    for (size_t i = 0; i < numFaces; ++i) {
        lightWeights[i] = 1.f / lightWeights[i];
    }
}

Scene::Scene(
    vector<Material>&& materials,
    vector<Mesh>&& meshes,
    AreaLights&& areaLights)
    : materials(move(materials))
    , meshes(move(meshes))
    , lights(move(areaLights))
{ 
    rtcScene = nullptr;
}

unsigned makeRTCMesh(RTCScene rtcScene, size_t i, const vector<Mesh>& meshes) {
    unsigned geomID = rtcNewTriangleMesh(
        rtcScene,
        RTC_GEOMETRY_STATIC,
        meshes[i].indices.size() / 3,
        meshes[i].vertices.size(),
        1);

    vec4* vbuffer = (vec4*) rtcMapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

    for (size_t j = 0; j < meshes[i].vertices.size(); ++j) {
        vbuffer[j].x = meshes[i].vertices[j].x;
        vbuffer[j].y = meshes[i].vertices[j].y;
        vbuffer[j].z = meshes[i].vertices[j].z;
        vbuffer[j].w = 1;
    }

    rtcUnmapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

    int* triangles = (int*) rtcMapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);
    std::memcpy(
        triangles,
        meshes[i].indices.data(),
        meshes[i].indices.size() * sizeof(int));

    rtcUnmapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);

    return geomID;
}

unsigned makeRTCMesh(RTCScene rtcScene, const AreaLights& lights) {
    unsigned geomID = rtcNewTriangleMesh(
        rtcScene,
        RTC_GEOMETRY_STATIC,
        lights.indices.size() / 3,
        lights.vertices.size(),
        1);

    vec4* vbuffer = (vec4*) rtcMapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

    for (size_t j = 0; j < lights.vertices.size(); ++j) {
        vbuffer[j].x = lights.vertices[j].x;
        vbuffer[j].y = lights.vertices[j].y;
        vbuffer[j].z = lights.vertices[j].z;
        vbuffer[j].w = 1;
    }

    rtcUnmapBuffer(rtcScene, geomID, RTC_VERTEX_BUFFER);

    int* triangles = (int*) rtcMapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);
    std::memcpy(
        triangles,
        lights.indices.data(),
        lights.indices.size() * sizeof(int));

    rtcUnmapBuffer(rtcScene, geomID, RTC_INDEX_BUFFER);

    return geomID;
}

void updateRTCScene(RTCScene& rtcScene, RTCDevice device, const Scene& scene) {
    if (rtcScene) {
        rtcDeleteScene(rtcScene);
    }

    rtcScene = rtcDeviceNewScene(
        device, 
        RTC_SCENE_STATIC | RTC_SCENE_HIGH_QUALITY, 
        RTC_INTERSECT1);

    if (rtcScene == nullptr) {
        throw std::runtime_error("Cannot create RTCScene.");
    }

    for (size_t i = 0; i < scene.meshes.size(); ++i) {
        unsigned geomID = makeRTCMesh(rtcScene, i, scene.meshes);

        if (geomID != i) {
            rtcDeleteScene(rtcScene);
            throw std::runtime_error("Geometry ID doesn't correspond to mesh index.");
        }
    }

    unsigned geomID = makeRTCMesh(rtcScene, scene.lights);

    if (geomID != scene.meshes.size()) {
        rtcDeleteScene(rtcScene);
        throw std::runtime_error("Geometry ID doesn't correspond to mesh index.");
    }

    rtcCommit(rtcScene);
}

void Scene::buildAccelStructs(RTCDevice device) {
    if (rtcScene == nullptr) {
        updateRTCScene(rtcScene, device, *this);
        lights.buildLightStructs();
    }
}

bool Scene::isMesh(const RayIsect& hit) const {
    return hit.geomID < meshes.size();
}

bool Scene::isLight(const RayIsect& isect) const {
    return isect.geomID == meshes.size();
}

const Material& Scene::material(const RayIsect& hit) const {
    return materials[meshes[hit.geomID].materialID];
}

vec3 Scene::lerpNormal(const RayIsect& hit) const {
    const float w = 1.f - hit.u - hit.v;
    auto& mesh = meshes[hit.geomID];

    return w * mesh.normals[mesh.indices[hit.primID * 3 + 0]] +
         hit.u * mesh.normals[mesh.indices[hit.primID * 3 + 1]] +
         hit.v * mesh.normals[mesh.indices[hit.primID * 3 + 2]];
}

RayIsect Scene::intersect(const vec3& origin, const vec3& direction) const {
    RayIsect rtcRay;
    (*(vec3*)rtcRay.org) = origin;
    (*(vec3*)rtcRay.dir) = direction;
    rtcRay.tnear = 0.f;
    rtcRay.tfar = INFINITY;
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.mask = 0xFFFFFFFF;
    rtcRay.time = 0.f;
    rtcIntersect(rtcScene, rtcRay);
    
    rtcRay.position = origin + direction * rtcRay.tfar;

    return rtcRay;
}

float Scene::occluded(const vec3& origin, const vec3& target) const {
    RTCRay rtcRay;
    (*(vec3*)rtcRay.org) = origin;
    (*(vec3*)rtcRay.dir) = target - origin;
    rtcRay.tnear = 0.0001f;
    rtcRay.tfar = 0.9999f;
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.mask = 0xFFFFFFFF;
    rtcRay.time = 0.f;
    rtcOccluded(rtcScene, rtcRay);
    return rtcRay.geomID == 0 ? 0.f : 1.f;
}

}
