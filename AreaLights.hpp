#pragma once
#include <Prerequisites.hpp>
#include <Geometry.hpp>
#include <utility.hpp>
#include <Intersector.hpp>
#include <SurfacePoint.hpp>

namespace haste {

struct Mesh {
    string name;
    uint32_t material_id;
    vector<int> indices;
    vector<vec3> vertices;
    vector<mat3> tangents;
};

struct LightSample {
    SurfacePoint surface;
    vec3 _radiance;
    float _areaDensity;

    const vec3& position() const { return surface.position(); }
    const vec3& normal() const { return surface.normal(); }
    const vec3& gnormal() const { return surface.gnormal; }
    const vec3& radiance() const { return _radiance; }
    const float areaDensity() const { return _areaDensity; };
};

struct LSDFQuery {
    vec3 radiance;
    float density;
};

struct AreaLight {
    vec3 position;
    mat3 tangent;
    vec2 size;
    vec3 exitance;
    uint32_t material_id;

    float area() const { return size.x * size.y; }
    float power() const { return area() * l1Norm(exitance); }
    float radius() const { return length(size) * 0.5f; }
    vec3 radiance() const { return exitance * one_over_pi<float>(); }
    vec3 normal() const { return tangent[1]; }

    std::unique_ptr<BSDF> create_bsdf(const bounding_sphere_t&, uint32_t light_id) const;
    Mesh create_mesh(uint32_t material_index, const string& name) const;
};

class AreaLights : public Geometry {
public:
    void init(const Intersector* intersector);

    const size_t addLight(
        const string& name,
        uint32_t material_index,
        const vec3& position,
        const vec3& direction,
        const vec3& up,
        const vec3& exitance,
        const vec2& size);

    const size_t num_lights() const;
    const string& name(size_t lightId) const;
    const AreaLight& light(size_t light_id) const;

    const float totalArea() const;
    const float totalPower() const;

    LightSample sample(RandomEngine& engine) const;

    vec3 queryRadiance(size_t lightId, const vec3& omega) const;

    LSDFQuery queryLSDF(size_t lightId, const vec3& omega) const;

    const mat3 light_to_world_mat3(size_t lightId) const;

    const bool castShadow() const override;
    const bool usesQuads() const override;
    const size_t numQuads() const override;
    void updateBuffers(int* indices, vec4* vertices) const override;
public:
    const Intersector* _intersector = nullptr;
    mutable piecewise_sampler_t _light_sampler;

    vector<string> _names;
    vector<AreaLight> _lights;
    vector<float> _weights;
    float _totalPower = 0.0f;
    float _totalArea = 0.0f;

    void _updateSampler();
    const size_t _sampleLight(RandomEngine& engine) const;
    const vec3 _samplePosition(size_t lightId, RandomEngine& engine) const;
};

}
