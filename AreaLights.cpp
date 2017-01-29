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

std::unique_ptr<BSDF> AreaLight::create_bsdf(
    const bounding_sphere_t& bounding_sphere,
    uint32_t light_id,
    bool diffuse) const {
    if (diffuse) {
        return std::unique_ptr<BSDF>(new LightBSDF(bounding_sphere, light_id));
    }
    else {
        return std::unique_ptr<BSDF>(new sun_light_bsdf(bounding_sphere, light_id));
    }
}

Mesh AreaLight::create_mesh(uint32_t material_index, const string& name) const {
    Mesh mesh;
    mesh.name = name;
    mesh.material_id = encode_material(material_index, entity_type::light);
    mesh.indices = { 0, 1, 2, 2, 3, 0 };
    mesh.vertices.resize(4);
    mesh.tangents.resize(4);

    vec3 left = tangent[0] * 0.5f;
    vec3 up = tangent[2] * 0.5f;

    mesh.vertices[0] = vec4(position - size.x * left - size.y * up, 1.0f);
    mesh.vertices[1] = vec4(position + size.x * left - size.y * up, 1.0f);
    mesh.vertices[2] = vec4(position + size.x * left + size.y * up, 1.0f);
    mesh.vertices[3] = vec4(position - size.x * left + size.y * up, 1.0f);

    mesh.tangents[0] = tangent;
    mesh.tangents[1] = tangent;
    mesh.tangents[2] = tangent;
    mesh.tangents[3] = tangent;

    return mesh;
}

void AreaLights::init(const Intersector* intersector) {
    _intersector = intersector;
}

const size_t AreaLights::addLight(
    const string& name,
    uint32_t material_index,
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
    light.material_id = encode_material(material_index, entity_type::light);

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
    random_generator_t& generator) const
{
    size_t light_id = _sampleLight(generator);
    const auto& light = this->light(light_id);

    LightSample result;

    result.surface._position = _samplePosition(light_id, generator);
    result.surface._tangent = light.tangent;
    result.surface.gnormal = light.normal();
    result.surface.material_id = light.material_id;

    result._radiance = light.radiance();
    result._areaDensity = _weights[light_id] / light.area();

    return result;
}

LSDFQuery AreaLights::queryLSDF(
    size_t light_id,
    const vec3& omega) const
{
    auto& light = this->light(light_id);

    float cosTheta = dot(omega, light.normal());

    LSDFQuery result;
    result.radiance = light.radiance() * (cosTheta > 0.0f ? 1.0f : 0.0f);
    result.density = _weights[light_id] / light.area();

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

    _light_sampler = piecewise_sampler_t(
        _weights.data(),
        _weights.data() + _weights.size());
}

const size_t AreaLights::_sampleLight(random_generator_t& generator) const {
    runtime_assert(num_lights() != 0);

    auto sample = _light_sampler.sample(generator);
    return min(size_t(sample * num_lights()), num_lights() - 1);
}

const vec3 AreaLights::_samplePosition(size_t lightId, random_generator_t& generator) const {
    auto sample = vec2(generator.sample(), generator.sample());
    auto uniform = (sample - vec2(0.5f)) * _lights[lightId].size;

    const vec3 up = _lights[lightId].tangent[0];
    const vec3 left = _lights[lightId].tangent[2];

    return _lights[lightId].position + uniform.x * left + uniform.y * up;
}

}
