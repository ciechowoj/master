#include <runtime_assert>
#include <Scene.hpp>
#include <streamops.hpp>
#include <cstring>

namespace haste {

Scene::Scene(
    Cameras&& cameras,
    Materials&& materials,
    vector<Mesh>&& meshes,
    AreaLights&& areaLights)
    : _cameras(cameras)
    , materials(move(materials))
    , meshes(move(meshes))
    , lights(move(areaLights))
{
    rtcScene = nullptr;

    _numIntersectRays = 0;
    _numOccludedRays = 0;

    lights.init(this);
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

    unsigned geomID = newMesh(rtcScene, scene.lights);
    runtime_assert(geomID == 0, "Area lights have to get 0 primID.");

    for (size_t i = 0; i < scene.meshes.size(); ++i) {
        unsigned geomID = makeRTCMesh(rtcScene, i, scene.meshes);
        runtime_assert(geomID == i + 1, "Geometry ID doesn't correspond to mesh index.");
    }

    rtcCommit(rtcScene);
}

void Scene::buildAccelStructs(RTCDevice device) {
    if (rtcScene == nullptr) {
        updateRTCScene(rtcScene, device, *this);
    }
}

const BSDF& Scene::queryBSDF(const RayIsect& isect) const {
    runtime_assert(isect.meshId() < meshes.size());
    return *materials.bsdfs[meshes[isect.meshId()].materialID];
}

vec3 Scene::lerpNormal(const RayIsect& hit) const {
    runtime_assert(hit.meshId() < meshes.size());

    const float w = 1.f - hit.u - hit.v;
    auto& mesh = meshes[hit.meshId()];

    return w * mesh.normals[mesh.indices[hit.primID * 3 + 0]] +
         hit.u * mesh.normals[mesh.indices[hit.primID * 3 + 1]] +
         hit.v * mesh.normals[mesh.indices[hit.primID * 3 + 2]];
}

SurfacePoint Scene::querySurface(const RayIsect& isect) const {
    runtime_assert(isect.meshId() < meshes.size());

    const float w = 1.f - isect.u - isect.v;
    auto& mesh = meshes[isect.meshId()];

    SurfacePoint point;
    point._position = (vec3&)isect.org + (vec3&)isect.dir * isect.tfar;

    point._tangent[0] =
        normalize(w * mesh.bitangents[mesh.indices[isect.primID * 3 + 0]] +
        isect.u * mesh.bitangents[mesh.indices[isect.primID * 3 + 1]] +
        isect.v * mesh.bitangents[mesh.indices[isect.primID * 3 + 2]]);

    point._tangent[1] =
        normalize(w * mesh.normals[mesh.indices[isect.primID * 3 + 0]] +
        isect.u * mesh.normals[mesh.indices[isect.primID * 3 + 1]] +
        isect.v * mesh.normals[mesh.indices[isect.primID * 3 + 2]]);

    point._tangent[2] =
        normalize(w * mesh.tangents[mesh.indices[isect.primID * 3 + 0]] +
        isect.u * mesh.tangents[mesh.indices[isect.primID * 3 + 1]] +
        isect.v * mesh.tangents[mesh.indices[isect.primID * 3 + 2]]);

    point._materialId = mesh.materialID;

    return point;
}

vec3 Scene::queryRadiance(const RayIsect& isect) const {
    return lights.lightRadiance(isect.primId());
}

LightSample Scene::sampleLight(
    RandomEngine& engine,
    const vec3& position) const
{
    return lights.sample(engine, position);
}

LightSampleEx Scene::sampleLightEx(
    RandomEngine& engine,
    const vec3& position) const
{
    return lights.sampleEx(engine, position);
}

vec3 Scene::queryRadiance(
    const RayIsect& isect,
    const vec3& omega) const
{
    return lights.queryRadiance(isect.primId(), omega);
}

const LSDFQuery Scene::queryLSDF(
    const RayIsect& isect,
    const vec3& omega) const
{
    return lights.queryLSDF(isect.primId(), omega);
}

const BSDFSample Scene::sampleBSDF(
    RandomEngine& engine,
    const SurfacePoint& surface,
    const vec3& omega) const
{
    runtime_assert(surface.materialId() < materials.bsdfs.size());

    auto bsdf = materials.bsdfs[surface.materialId()].get();
    return bsdf->sample(engine, surface, omega);
}

const vec3 Scene::queryBSDF(
    const SurfacePoint& surface,
    const vec3& incident,
    const vec3& outgoing) const
{
    runtime_assert(surface.materialId() < materials.bsdfs.size());

    auto bsdf = materials.bsdfs[surface.materialId()].get();
    return bsdf->query(surface, incident, outgoing);
}

const BSDFQuery Scene::queryBSDFEx(
    const SurfacePoint& surface,
    const vec3& incident,
    const vec3& outgoing) const
{
    runtime_assert(surface.materialId() < materials.bsdfs.size());

    auto bsdf = materials.bsdfs[surface.materialId()].get();
    return bsdf->queryEx(surface, incident, outgoing);
}

const RayIsect Scene::intersect(
    const vec3& origin,
    const vec3& direction) const
{
    RayIsect rtcRay;
    (*(vec3*)rtcRay.org) = origin;
    (*(vec3*)rtcRay.dir) = direction;
    rtcRay.tnear = 0.00001f;
    rtcRay.tfar = INFINITY;
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.mask = 0xFFFFFFFF;
    rtcRay.time = 0.f;
    rtcIntersect(rtcScene, rtcRay);

    ++_numIntersectRays;

    return rtcRay;
}

const float Scene::occluded(
    const vec3& origin,
    const vec3& target) const
{
    RTCRay rtcRay;
    (*(vec3*)rtcRay.org) = origin;
    (*(vec3*)rtcRay.dir) = target - origin;
    rtcRay.tnear = 0.00001f;
    rtcRay.tfar = 0.99999f;
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.mask = RayIsect::occluderMask();
    rtcRay.time = 0.f;
    rtcOccluded(rtcScene, rtcRay);

    ++_numOccludedRays;

    return rtcRay.geomID == 0 ? 0.f : 1.f;
}

const RayIsect Scene::intersect(const Ray& ray) const {
    return intersect(ray.origin, ray.direction);
}

const RayIsect Scene::intersectLight(
    const vec3& origin,
    const vec3& direction) const
{
    RayIsect rtcRay;
    (*(vec3*)rtcRay.org) = origin;
    (*(vec3*)rtcRay.dir) = direction;
    rtcRay.tnear = 0.0001f;
    rtcRay.tfar = INFINITY;
    rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
    rtcRay.mask = RayIsect::lightMask();
    rtcRay.time = 0.f;
    rtcIntersect(rtcScene, rtcRay);

    ++_numIntersectRays;

    return rtcRay;
}

const size_t Scene::numNormalRays() const {
    return _numIntersectRays;
}

const size_t Scene::numShadowRays() const {
    return _numOccludedRays;
}

const size_t Scene::numRays() const {
    return _numIntersectRays + _numOccludedRays;
}

const LightSampleEx Scene::sampleLight(
        RandomEngine& engine) const
{
    return lights.sample(engine);
}

const vec3 Scene::sampleDirectLightAngle(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& omegaR,
    const BSDF& bsdf) const
{
    auto bsdfSample = bsdf.sample(engine, point, omegaR);

    Ray ray = { point.position(), bsdfSample.omega() };
    RayIsect isect = intersect(ray);

    vec3 radiance = vec3(0.0f);

    while (isect.isLight()) {
        radiance +=
            lights.lightRadiance(isect.primId()) *
            bsdfSample.throughput() *
            dot(bsdfSample.omega(), point.normal()) *
            bsdfSample.densityInv() *
            (dot(-bsdfSample.omega(), lights.lightNormal(isect.primId())) > 0.0f ? 1.0f : 0.0f);

        ray.origin = isect.position();
        isect = intersect(ray);
    }

    return radiance;
}

const vec3 Scene::sampleDirectLightArea(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& omegaR,
    const BSDF& bsdf) const
{
    LightSample lightSample = lights.sample(engine, point.position());
    const float cosTheta = dot(-lightSample.omega(), point.normal());

    return
        lightSample.radiance() *
        bsdf.query(point, omegaR, -lightSample.omega()) *
        (cosTheta > 0.0f ? 1.0f : 0.0f) *
        cosTheta *
        lightSample.densityInv();
}

const vec3 Scene::sampleDirectLightMixed(
    RandomEngine& engine,
    const SurfacePoint& point,
    const vec3& omegaR,
    const BSDF& bsdf) const
{
    auto bsdfSample = bsdf.sample(engine, point, omegaR);

    Ray ray = { point.position(), bsdfSample.omega() };
    RayIsect isect = intersect(ray);

    vec3 bsdfRadiance = vec3(0.0f);

    while (isect.isLight()) {
        bsdfRadiance +=
            lights.lightRadiance(isect.primId()) *
            bsdfSample.throughput() *
            dot(bsdfSample.omega(), point.normal()) *
            (dot(-bsdfSample.omega(), lights.lightNormal(isect.primId())) > 0.0f ? 1.0f : 0.0f);

        ray.origin = isect.position();
        isect = intersect(ray);
    }

    LightSample lightSample = lights.sample(engine, point.position());
    float distSqInv = 1.0f / distance2(lightSample.position(), point.position());
    float fCosTheta = abs(dot(lightSample.omega(), lightSample.normal()));
    float bCosTheta = dot(-lightSample.omega(), point.normal());

    const vec3 lightRadiance =
        lightSample.radiance() *
        bsdf.query(point, omegaR, -lightSample.omega()) *
        bCosTheta *
        fCosTheta *
        distSqInv *
        occluded(lightSample.position(), point.position()) *
        (bCosTheta > 0.0f ? vec3(1.0f) : vec3(0.0f));

    const float d = bsdfSample.density() + lightSample.density();

    return
        bsdfRadiance /
            (bsdfSample.density()
                + lights.density(point.position(), bsdfSample.omega())) +
        lightRadiance /
            (lightSample.density()
                + bsdf.density(point, -lightSample.omega(), omegaR));
}

}
