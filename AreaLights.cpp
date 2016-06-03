#include <streamops.hpp>
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

void AreaLights::init(const Intersector* intersector) {
    _intersector = intersector;
}

const size_t AreaLights::addLight(
    const string& name,
    const vec3& position,
    const vec3& direction,
    const vec3& up,
    const vec3& exitance,
    const vec2& size)
{
    size_t lightId = _names.size();

    _names.push_back(name);

    Shape shape;
    shape.position = position;
    shape.direction = direction;
    shape.up = up;

    _shapes.push_back(shape);
    _sizes.push_back(size);
    _exitances.push_back(exitance);

    _totalPower += lightPower(lightId);
    _totalArea += lightArea(lightId);

    _updateSampler();

    return lightId;
}

const size_t AreaLights::numLights() const {
    return _names.size();
}

const string& AreaLights::name(size_t lightId) const {
    runtime_assert(lightId < _names.size());
    return _names[lightId];
}

const float AreaLights::lightArea(size_t lightId) const {
    runtime_assert(lightId < _names.size());
    return _sizes[lightId].x * _sizes[lightId].y;
}

const float AreaLights::lightPower(size_t lightId) const {
    runtime_assert(lightId < _names.size());
    auto exitance = _exitances[lightId];
    return lightArea(lightId) * (exitance.x + exitance.y + exitance.z);
}

const vec3 AreaLights::lightNormal(size_t lightId) const {
    runtime_assert(lightId < _names.size());
    return _shapes[lightId].direction;
}

const vec3 AreaLights::lightRadiance(size_t lightId) const {
    runtime_assert(lightId < _names.size());
    return  _exitances[lightId] * one_over_pi<float>();
}

const float AreaLights::totalArea() const {
    return _totalArea;
}

const float AreaLights::totalPower() const {
    return _totalPower;
}

const vec3 AreaLights::toWorld(size_t lightId, const vec3& omega) const {
    const vec3 up = _shapes[lightId].up;
    const vec3 left = normalize(cross(up, _shapes[lightId].direction));

    mat3 matrix;
    matrix[0] = left;
    matrix[1] = _shapes[lightId].direction;
    matrix[2] = up;

    return matrix * omega;
}

Photon AreaLights::emit(RandomEngine& engine) const {
    size_t lightId = _sampleLight(engine);
    vec3 position = _samplePosition(lightId, engine);

    Photon result;
    result.position = position;
    result.direction = toWorld(lightId, sampleCosineHemisphere1(engine).omega());
    result.power = _exitances[lightId];
    float length1 = 1.f / (result.power.x + result.power.y + result.power.z);
    result.power = result.power * length1;

    return result;
}

const float AreaLights::density(
    const vec3& position,
    const vec3& direction) const
{
    runtime_assert(_intersector != nullptr);

    auto isect = _intersector->intersect(position, direction);

    while (isect.isLight()) {
        const auto lightId = isect.primId();
        const float cosTheta = -dot(direction, _shapes[lightId].direction);

        if (cosTheta > 0.0f) {
            return _weights[isect.primId()] * distance2(position, isect.position())
                / (lightArea(lightId) * cosTheta);
        }

        isect = _intersector->intersect(isect.position(), direction);
    }

    return 0.0f;
}

LightSampleEx AreaLights::sample(
    RandomEngine& engine) const
{
    size_t lightId = _sampleLight(engine);
    auto sample = sampleCosineHemisphere1(engine);

    LightSampleEx result;
    result._position = _samplePosition(lightId, engine);
    result._normal = lightNormal(lightId);
    result._omega = toWorld(lightId, sample.omega());
    result._radiance = lightRadiance(lightId);
    result._areaDensity = _weights[lightId] / lightArea(lightId);
    result._omegaDensity = sample.density();

    return result;
}

LightSample AreaLights::sample(
    RandomEngine& engine,
    const vec3& position) const
{
    size_t lightId = _sampleLight(engine);

    LightSample result;
    result._position = _samplePosition(lightId, engine);
    result._normal = _shapes[lightId].direction;
    result._radiance = lightRadiance(lightId);
    result._omega = normalize(position - result._position);
    result._density = _weights[lightId] / lightArea(lightId);

    float cosTheta = dot(result.omega(), result.normal());

    result._radiance *=
        _intersector->occluded(result.position(), position) *
        (cosTheta > 0.0f ? 1.0f : 0.0f);

    return result;
}

LightSampleEx AreaLights::sampleEx(
    RandomEngine& engine,
    const vec3& position) const
{
    size_t lightId = _sampleLight(engine);

    LightSampleEx result;
    result._position = _samplePosition(lightId, engine);
    result._normal = _shapes[lightId].direction;
    result._radiance = lightRadiance(lightId);
    result._omega = normalize(position - result._position);
    result._areaDensity = _weights[lightId] / lightArea(lightId);
    result._omegaDensity = dot(result.omega(), result.normal());

    float cosTheta = result._omegaDensity;

    result._radiance *=
        _intersector->occluded(result.position(), position) *
        (cosTheta > 0.0f ? 1.0f : 0.0f);

    return result;
}

vec3 AreaLights::queryRadiance(
    size_t lightId,
    const vec3& omega) const
{
    float cosTheta = dot(omega, lightNormal(lightId));
    return lightRadiance(lightId) * (cosTheta > 0.0f ? 1.0f : 0.0f);
}

LSDFQuery AreaLights::queryLSDF(
    size_t lightId,
    const vec3& omega) const
{
    float cosTheta = dot(omega, lightNormal(lightId));

    LSDFQuery result;
    result._radiance = lightRadiance(lightId) * (cosTheta > 0.0f ? 1.0f : 0.0f);
    result._areaDensity = _weights[lightId] / lightArea(lightId);
    result._omegaDensity = abs(cosTheta);
    return result;
}

const bool AreaLights::castShadow() const {
    return false;
}

const bool AreaLights::usesQuads() const {
    return true;
}

const size_t AreaLights::numQuads() const {
    return numLights();
}

void AreaLights::updateBuffers(int* indices, vec4* vertices) const {
    const int numIndices = int(this->numIndices());

    for (int i = 0; i < numIndices; ++i) {
        indices[i] = i;
    }

    const size_t numQuads = this->numLights();

    for (size_t i = 0; i < numQuads; ++i) {
        const vec3 up = _shapes[i].up * 0.5f;
        const vec3 left = normalize(cross(up, _shapes[i].direction)) * 0.5f;
        const vec3 position = _shapes[i].position;

        vertices[i * 4 + 0] =
            vec4(position - _sizes[i].x * left - _sizes[i].y * up, 1.0f);
        vertices[i * 4 + 1] =
            vec4(position + _sizes[i].x * left - _sizes[i].y * up, 1.0f);
        vertices[i * 4 + 2] =
            vec4(position + _sizes[i].x * left + _sizes[i].y * up, 1.0f);
        vertices[i * 4 + 3] =
            vec4(position - _sizes[i].x * left + _sizes[i].y * up, 1.0f);
    }
}

void AreaLights::_updateSampler() {
    const float totalPower = this->totalPower();
    const float totalPowerInv = 1.0f / totalPower;
    const size_t numLights = this->numLights();

    _weights.resize(numLights);

    for (size_t i = 0; i < numLights; ++i) {
        const float power = lightPower(i);
        _weights[i] = power * totalPowerInv;
    }

    lightSampler = PiecewiseSampler(
        _weights.data(),
        _weights.data() + _weights.size());

    /*for (size_t i = 0; i < numLights; ++i) {
        _weights[i] = 1.f / _weights[i];
    }*/
}

const size_t AreaLights::_sampleLight(RandomEngine& engine) const {
    runtime_assert(numLights() != 0);

    auto sample = lightSampler.sample();
    return min(size_t(sample * numLights()), numLights() - 1);
}

const vec3 AreaLights::_samplePosition(size_t lightId, RandomEngine& engine) const {
    auto uniform =
        (sampleUniform2(engine).value() - vec2(0.5f)) * _sizes[lightId];

    const vec3 up = _shapes[lightId].up;
    const vec3 left = normalize(cross(up, _shapes[lightId].direction));

    return _shapes[lightId].position + uniform.x * left + uniform.y * up;
}

}
