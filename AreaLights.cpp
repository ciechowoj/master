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

void AreaLights::init(const Intersector* intersector, bounding_sphere_t sphere) {
    _intersector = intersector;
    _scene_bound = sphere;
}

const size_t AreaLights::addLight(
    const string& name,
    int32_t materialId,
    const vec3& position,
    const vec3& direction,
    const vec3& up,
    const vec3& exitance,
    const vec2& size)
{
    size_t lightId = _names.size();

    AreaLight light;
    light.position = position;
    light.tangent[0] = normalize(cross(up, direction));
    light.tangent[1] = direction;
    light.tangent[2] = up;
    light.size = size;
    light.exitance = exitance;
    light.materialId = materialId;

    _names.push_back(name);
    _lights.push_back(light);

    _totalPower += light.power();
    _totalArea += light.area();

    _updateSampler();

    return lightId;
}

const size_t AreaLights::num_lights() const {
    return _names.size();
}

const string& AreaLights::name(size_t light_id) const {
    runtime_assert(light_id < _names.size());
    return _names[light_id];
}

const AreaLight& AreaLights::light(size_t light_id) const {
    runtime_assert(light_id < _names.size());
    return _lights[light_id];
}

const float AreaLights::totalArea() const {
    return _totalArea;
}

const float AreaLights::totalPower() const {
    return _totalPower;
}

LightSample AreaLights::sample(
    RandomEngine& engine) const
{
    size_t light_id = _sampleLight(engine);
    const auto& light = this->light(light_id);

    bounding_sphere_t bound = _scene_bound;
    bound.center = (bound.center - light.position) * light.tangent;
    bound.radius += light.radius();

    auto sample = sample_lambert(engine, vec3(0.0f, 1.0f, 0.0f), bound);

    LightSample result;

    result.surface._position = _samplePosition(light_id, engine);
    result.surface._tangent = light.tangent;
    result.surface.gnormal = light.normal();
    result.surface._materialId = light.materialId;

    result._omega = light.tangent * sample.direction;
    result._radiance = light.radiance();
    result._areaDensity = _weights[light_id] / light.area();
    result._omegaDensity = lambert_density(sample);

    return result;
}

vec3 AreaLights::queryRadiance(
    size_t light_id,
    const vec3& omega) const
{
    auto& light = this->light(light_id);

    float cosTheta = dot(omega, light.normal());
    return light.radiance() * (cosTheta > 0.0f ? 1.0f : 0.0f);
}

LSDFQuery AreaLights::queryLSDF(
    size_t light_id,
    const vec3& omega) const
{
    auto& light = this->light(light_id);

    float cosTheta = dot(omega, light.normal());

    LSDFQuery result;
    result._radiance = light.radiance() * (cosTheta > 0.0f ? 1.0f : 0.0f);
    result._areaDensity = _weights[light_id] / light.area();
    result._omegaDensity = abs(cosTheta) * one_over_pi<float>();

    return result;
}

const mat3 AreaLights::light_to_world_mat3(size_t lightId) const {
    return light(lightId).tangent;
}

const bool AreaLights::castShadow() const {
    return false;
}

const bool AreaLights::usesQuads() const {
    return true;
}

const size_t AreaLights::numQuads() const {
    return num_lights();
}

void AreaLights::updateBuffers(int* indices, vec4* vertices) const {
    const int numIndices = int(this->numIndices());

    for (int i = 0; i < numIndices; ++i) {
        indices[i] = i;
    }

    const size_t numQuads = this->num_lights();

    for (size_t i = 0; i < numQuads; ++i) {
        const vec3 up = _lights[i].tangent[0] * 0.5f;
        const vec3 left = _lights[i].tangent[2] * 0.5f;
        const vec3 position = _lights[i].position;
        const vec2 size = _lights[i].size;

        vertices[i * 4 + 0] =
            vec4(position - size.x * left - size.y * up, 1.0f);
        vertices[i * 4 + 1] =
            vec4(position + size.x * left - size.y * up, 1.0f);
        vertices[i * 4 + 2] =
            vec4(position + size.x * left + size.y * up, 1.0f);
        vertices[i * 4 + 3] =
            vec4(position - size.x * left + size.y * up, 1.0f);
    }
}

void AreaLights::_updateSampler() {
    const float totalPower = this->totalPower();
    const float totalPowerInv = 1.0f / totalPower;
    const size_t num_lights = this->num_lights();

    _weights.resize(num_lights);

    for (size_t i = 0; i < num_lights; ++i) {
        const float power = light(i).power();
        _weights[i] = power * totalPowerInv;
    }

    lightSampler = PiecewiseSampler(
        _weights.data(),
        _weights.data() + _weights.size());
}

const size_t AreaLights::_sampleLight(RandomEngine& engine) const {
    runtime_assert(num_lights() != 0);

    auto sample = lightSampler.sample();
    return min(size_t(sample * num_lights()), num_lights() - 1);
}

const vec3 AreaLights::_samplePosition(size_t lightId, RandomEngine& engine) const {
    auto sample = vec2(engine.sample(), engine.sample());
    auto uniform = (sample - vec2(0.5f)) * _lights[lightId].size;

    const vec3 up = _lights[lightId].tangent[0];
    const vec3 left = _lights[lightId].tangent[2];

    return _lights[lightId].position + uniform.x * left + uniform.y * up;
}

}
