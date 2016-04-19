#include <runtime_assert>
#include <Scene.hpp>

namespace haste {

//
// Light space.
//
//          n/y
//           +
//           |
//           |
//           |
//           | v0
//           /-----------+ b/x
//          /   ****
//         /        ****
//        /             ****
//       +                  ****
//      t/z                     ****
//     *                            ****
// v1 ************************************** v2
//

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
    return length(exitances[face]) * faceArea(face) * pi<float>();
}

vec3 AreaLights::lerpPosition(size_t face, vec3 uvw) const {
    return vertices[indices[face * 3 + 0]] * uvw.z
        + vertices[indices[face * 3 + 1]] * uvw.x
        + vertices[indices[face * 3 + 2]] * uvw.y;
}

float AreaLights::queryTotalPower() const {
    vec3 power = queryTotalPower3();
    return power.x + power.y + power.z;
}

vec3 AreaLights::queryTotalPower3() const {
    runtime_assert(indices.size() % 3 == 0);

    vec3 result(0.0f);

    size_t numFaces = this->numFaces();

    for (size_t i = 0; i < numFaces; ++i) {
        result += queryAreaLightPower3(i);
    }

    return result;
}

float AreaLights::queryAreaLightPower(size_t id) const {
    vec3 power = queryAreaLightPower3(id);
    return power.x + power.y + power.z;
}

vec3 AreaLights::queryAreaLightPower3(size_t id) const {
    return exitances[id] * queryAreaLightArea(id);
}

float AreaLights::queryAreaLightArea(size_t id) const {
    vec3 u = vertices[indices[id * 3 + 1]] - vertices[indices[id * 3 + 0]];
    vec3 v = vertices[indices[id * 3 + 2]] - vertices[indices[id * 3 + 0]];
    return length(cross(u, v)) * 0.5f;
}

const vec3& AreaLights::faceNormal(size_t faceId) const {
    return toWorldMs[indices[faceId * 3 + 0]][1];
}

const vec3 AreaLights::faceRadiance(size_t faceId) const {
    return exitances[faceId] * one_over_pi<float>();
}

const vec3 AreaLights::faceRadiance(const RayIsect& isect) const {
    runtime_assert(isect.isLight());

    const vec3 exitance = exitances[isect.faceId()];

    if (dot(isect.normal(), isect.omega()) > 0.0f) {
        return exitance * one_over_pi<float>();
    }
    else {
        return vec3(0.0f);
    }
}

size_t AreaLights::sampleFace() const {
    runtime_assert(numFaces() != 0);
    auto sample = lightSampler.sample();

    return min(size_t(sample * numFaces()), numFaces() - 1);
}

LightPoint AreaLights::sampleSurface(size_t id) const {
    LightPoint result;

    vec3 uvw = faceSampler.sample();

    result.position =
        vertices[indices[id * 3 + 0]] * uvw.z +
        vertices[indices[id * 3 + 1]] * uvw.x +
        vertices[indices[id * 3 + 2]] * uvw.y;

    result.toWorldM =
        toWorldMs[indices[id * 3 + 0]] * uvw.z +
        toWorldMs[indices[id * 3 + 1]] * uvw.x +
        toWorldMs[indices[id * 3 + 2]] * uvw.y;

    result.toWorldM[0] = normalize(result.toWorldM[0]);
    result.toWorldM[1] = normalize(result.toWorldM[1]);
    result.toWorldM[2] = normalize(result.toWorldM[2]);

    return result;
}

Photon AreaLights::emit() const {
    size_t id = sampleFace();

    LightPoint point = sampleSurface(id);

    Photon result;
    result.position = point.position;
    result.direction = point.toWorldM * sampleCosineHemisphere1(source).omega();
    result.power = exitances[id];
    float length1 = 1.f / (result.power.x + result.power.y + result.power.z);
    result.power = result.power * length1;

    return result;
}

LightSample AreaLights::sample(
    RandomEngine& engine,
    const vec3& position) const
{
    size_t light = sampleFace();
    auto barycentric = sampleBarycentric1(engine);

    vec3 lightNormal = faceNormal(light);

    LightSample result;
    result._position = lerpPosition(light, barycentric.value());
    result._omega = normalize(result._position - position);
    float cosineTheta = dot(-result.omega(), lightNormal);
    vec3 diff = position - result.position();

    const float numerator = 1; // dot(diff, diff);
    const float denominator = lightWeights[light] * queryAreaLightArea(light) ;

    result._density = numerator / denominator;

    if (cosineTheta > 0.0f) {
        result._radiance = exitances[light] * one_over_pi<float>() / dot(diff, diff) * cosineTheta;
    }
    else {
        result._radiance = vec3(0.0f);
    }

    return result;
}

void AreaLights::buildLightStructs() const {
    size_t numFaces = this->numFaces();
    lightWeights.resize(numFaces);

    float totalPower = queryTotalPower();
    float totalPowerInv = 1.0f / totalPower;

    for (size_t i = 0; i < numFaces; ++i) {
        float power = queryAreaLightPower(i);
        lightWeights[i] = power * totalPowerInv;
    }

    lightSampler = PiecewiseSampler(
        lightWeights.data(),
        lightWeights.data() + lightWeights.size());

    for (size_t i = 0; i < numFaces; ++i) {
        lightWeights[i] = 1.f / lightWeights[i];
    }
}

}
