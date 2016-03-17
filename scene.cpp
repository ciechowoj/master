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
    return length(wattages[face]);
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

    rtcScene = rtcDeviceNewScene(device, RTC_SCENE_STATIC, RTC_INTERSECT1);

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

    unsigned geomID = makeRTCMesh(rtcScene, scene.areaLights);

    if (geomID != scene.meshes.size()) {
        rtcDeleteScene(rtcScene);
        throw std::runtime_error("Geometry ID doesn't correspond to mesh index.");
    }

    rtcCommit(rtcScene);
}

void updateLightCache(LightCache& cache, const AreaLights& lights) {
    size_t numFaces = lights.numFaces();
    cache.weights.resize(numFaces);

    for (size_t i = 0; i < numFaces; ++i) {
        cache.weights[i] = lights.faceArea(i) * lights.facePower(i);
    }

    cache.lightsSampler = PiecewiseSampler(
        cache.weights.data(), 
        cache.weights.data() + cache.weights.size());
}

LightSample LightCache::sampleLight() {
    size_t face = size_t(lightsSampler.sample() * lights->numFaces());
    vec3 uvw = faceSampler.sample();

    return LightSample { 
        lights->wattages[face], 
        lights->lerpPosition(face, uvw),
        lights->lerpNormal(face, uvw)
    };
}

void updateCache(SceneCache& cache, RTCDevice device, const Scene& scene) {
    updateRTCScene(cache.rtcScene, device, scene);
    updateLightCache(cache.lightCache, scene.areaLights);
}

}
