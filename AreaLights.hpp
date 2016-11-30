#pragma once
#include <Prerequisites.hpp>
#include <Geometry.hpp>
#include <utility.hpp>
#include <Intersector.hpp>
#include <SurfacePoint.hpp>

namespace haste {

struct LightSample {
    vec3 _position;
    vec3 _normal;
    vec3 _radiance; // with respect to omega
    vec3 _omega;
    float _density;

    const vec3& position() const { return _position; }
    const vec3& normal() const { return _normal; }
    const vec3& gnormal() const { return _normal; }
    const vec3& radiance() const { return _radiance; }
    const vec3& omega() const { return _omega; }
    const float density() const { return _density; }
    const float densityInv() const { return 1.0f / _density; }
};

struct LightSampleEx {
    vec3 _position;
    vec3 _normal;
    vec3 _tangent;
    vec3 _radiance; // with respect to omega
    vec3 _omega;
    float _areaDensity;
    float _omegaDensity;
    int32_t _materialId;

    const vec3& position() const { return _position; }
    const vec3& normal() const { return _normal; }
    const vec3& gnormal() const { return _normal; }
    const vec3& radiance() const { return _radiance; }
    const vec3& omega() const { return _omega; } // outgoing from light
    const float density() const { return _areaDensity * _omegaDensity; }
    const float densityInv() const { return 1.0f / density(); }
    const float areaDensity() const { return _areaDensity; };
    const float omegaDensity() const { return _omegaDensity; };
    const SurfacePoint surface() const;
};

struct LSDFQuery {
    vec3 _radiance;
    float _areaDensity;
    float _omegaDensity;

    const vec3& radiance() const { return _radiance; }
    const float areaDensity() const { return _areaDensity; }
    const float omegaDensity() const { return _omegaDensity; }
};

class AreaLights : public Geometry {
public:
    void init(const Intersector* intersector, bounding_sphere_t sphere);

    const size_t addLight(
        const string& name,
        int32_t _materialId,
        const vec3& position,
        const vec3& direction,
        const vec3& up,
        const vec3& exitance,
        const vec2& size);

    const size_t numLights() const;
    const string& name(size_t lightId) const;
    const float lightArea(size_t lightId) const;
    const float lightPower(size_t lightId) const;
    const vec3 lightNormal(size_t lightId) const;
    const vec3 lightRadiance(size_t lightId) const;
    const float totalArea() const;
    const float totalPower() const;

    const vec3 toWorld(size_t lightId, const vec3& omega) const;

    const float density(
        const vec3& position,
        const vec3& direction) const;

    LightSampleEx sample(
        RandomEngine& engine) const;

    LightSample sample(
        RandomEngine& engine,
        const vec3& position) const;

    vec3 queryRadiance(
        size_t lightId,
        const vec3& omega) const;

    LSDFQuery queryLSDF(
        size_t lightId,
        const vec3& omega) const;

    const mat3 light_to_world_mat3(size_t lightId) const;

    const bool castShadow() const override;
    const bool usesQuads() const override;
    const size_t numQuads() const override;
    void updateBuffers(int* indices, vec4* vertices) const override;
public:
    const Intersector* _intersector = nullptr;
    mutable PiecewiseSampler lightSampler;

    struct Shape {
        vec3 position;
        vec3 direction;
        vec3 up;
    };

    vector<string> _names;
    vector<int32_t> _materialIds;
    vector<Shape> _shapes;
    vector<vec2> _sizes;
    vector<vec3> _exitances;
    vector<float> _weights;
    float _totalPower = 0.0f;
    float _totalArea = 0.0f;

    void _updateSampler();
    const size_t _sampleLight(RandomEngine& engine) const;
    const vec3 _samplePosition(size_t lightId, RandomEngine& engine) const;
};

}
