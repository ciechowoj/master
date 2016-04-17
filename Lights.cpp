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

size_t Lights::numLights() const {
    return names.size();
}

size_t Lights::numFaces() const {
    return indices.size() / 3;
}

float Lights::faceArea(size_t face) const {
    vec3 u = vertices[indices[face * 3 + 1]] - vertices[indices[face * 3 + 0]];
    vec3 v = vertices[indices[face * 3 + 2]] - vertices[indices[face * 3 + 0]];
    return length(cross(u, v)) * 0.5f;
}

float Lights::facePower(size_t face) const {
    return length(exitances[face]) * faceArea(face) * pi<float>();
}

vec3 Lights::lerpPosition(size_t face, vec3 uvw) const {
    return vertices[indices[face * 3 + 0]] * uvw.z
        + vertices[indices[face * 3 + 1]] * uvw.x
        + vertices[indices[face * 3 + 2]] * uvw.y;
}

vec3 Lights::lerpNormal(size_t face, vec3 uvw) const {
    return normalize(toWorldMs[indices[face * 3 + 0]][1] * uvw.z
        + toWorldMs[indices[face * 3 + 1]][1] * uvw.x
        + toWorldMs[indices[face * 3 + 2]][1] * uvw.y);
}

vec3 Lights::lerpNormal(const RayIsect& hit) const {
    float w = 1.0f - hit.u - hit.v;

    return normalize(toWorldMs[indices[hit.primID * 3 + 0]][1] * w
        + toWorldMs[indices[hit.primID * 3 + 1]][1] * hit.u
        + toWorldMs[indices[hit.primID * 3 + 2]][1] * hit.v);
}

vec3 Lights::eval(const RayIsect& isect) const {
    return exitances[isect.primID] * dot(isect.gnormal(), isect.incident());
}

float Lights::queryTotalPower() const {
    vec3 power = queryTotalPower3();
    return power.x + power.y + power.z;
}

vec3 Lights::queryTotalPower3() const {
    runtime_assert(indices.size() % 3 == 0);

    vec3 result(0.0f);

    size_t numFaces = this->numFaces();

    for (size_t i = 0; i < numFaces; ++i) {
        result += queryAreaLightPower3(i);
    }

    return result;
}

float Lights::queryAreaLightPower(size_t id) const {
    vec3 power = queryAreaLightPower3(id);
    return power.x + power.y + power.z;
}

vec3 Lights::queryAreaLightPower3(size_t id) const {
    return exitances[id] * queryAreaLightArea(id);
}

float Lights::queryAreaLightArea(size_t id) const {
    vec3 u = vertices[indices[id * 3 + 1]] - vertices[indices[id * 3 + 0]];
    vec3 v = vertices[indices[id * 3 + 2]] - vertices[indices[id * 3 + 0]];
    return length(cross(u, v)) * 0.5f;
}

size_t Lights::sampleLight() const {
    runtime_assert(numFaces() != 0);
    auto sample = lightSampler.sample();
    return min(size_t(sample * numFaces()), numFaces() - 1);
}

LightPoint Lights::sampleSurface(size_t id) const {
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

Photon Lights::emit() const {
    size_t id = sampleLight();

    LightPoint point = sampleSurface(id);

    Photon result;
    result.position = point.position;
    result.direction = point.toWorldM * sampleCosineHemisphere1(source).omega();
    result.power = exitances[id];
    float length1 = 1.f / (result.power.x + result.power.y + result.power.z);
    result.power = result.power * length1;

    return result;
}

LightSample Lights::sample(const vec3& position) const {
    // below computations are probably incorrect (to be fixed)

    size_t face = sampleLight();

    vec3 uvw = faceSampler.sample();

    vec3 normal = lerpNormal(face, uvw);
    vec3 radiance = exitances[face] * one_over_pi<float>();

    LightSample sample;
    sample._position = lerpPosition(face, uvw);
    sample._omega = normalize(sample.position() - position);
    sample._radiance = max(vec3(0.0f), radiance * dot(normal, -sample.omega()));

    return sample;
}

LightSample Lights::sample(
    RandomEngine& engine,
    const vec3& position) const
{
    size_t light = sampleLight();
    auto barycentric = sampleBarycentric1(engine);

    vec3 lightNormal = lerpNormal(light, barycentric.value());

    LightSample result;
    result._position = lerpPosition(light, barycentric.value());
    result._omega = normalize(result._position - position);
    float cosineTheta = dot(-result.omega(), lightNormal);
    vec3 diff = position - result.position();
    result._density = 1.0f / (lightWeights[light] * queryAreaLightArea(light) * cosineTheta) * dot(diff, diff);

    if (cosineTheta > 0.0f) {
        result._radiance = exitances[light] * one_over_pi<float>();
    }
    else {
        result._radiance = vec3(0.0f);
    }

    return result;
}

void Lights::buildLightStructs() const {
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

void renderPhotons(
    vector<vec4>& image,
    size_t width,
    const vector<Photon>& photons,
    const mat4& proj)
{
    size_t height = image.size() / width;
    float f5width = 0.5f * float(width);
    float f5height = 0.5f * float(height);
    int radius = 0;

    for (size_t k = 0; k < photons.size(); ++k) {
        vec4 h = proj * vec4(photons[k].position, 1.0);
        vec3 v = h.xyz() / h.w;

        if (-1.0f <= v.z && v.z <= +1.0f) {
            int x = int((v.x + 1.0f) * f5width + 0.5f);
            int y = int((v.y + 1.0f) * f5height + 0.5f);

            for (int j = y - radius; j <= y + radius; ++j) {
                for (int i = x - radius; i <= x + radius; ++i) {
                    if (0 <= i && i < width && 0 <= j && j < height) {
                        vec3 power = photons[k].power * float(photons.size()) * 0.1f;
                        power = clamp(power, 0.0f, 1.0f);
                        image[j * width + i] = vec4(image[j * width + i].xyz() + power, 1.0f);
                    }
                }
            }
        }
    }
}

}
