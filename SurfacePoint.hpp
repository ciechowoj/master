#pragma once
#include <cstdint>

namespace haste {

using std::uint32_t;

struct SurfacePoint {
    vec3 _position;
    vec3 gnormal;
    mat3 _tangent;
    int32_t _materialId = 0;

    SurfacePoint() = default;

    SurfacePoint(
        const vec3& position,
        const vec3& normal,
        const vec3& tangent,
        int32_t materialId)
    {
        _position = position;
        _tangent[0] = normalize(cross(normal, tangent));
        _tangent[1] = normal;
        _tangent[2] = tangent;
        _materialId = materialId;
    }

    const vec3& position() const { return _position; }
    const vec3& normal() const { return _tangent[1]; }
    const vec3& tangent() const { return _tangent[2]; }
    const vec3& bitangent() const { return _tangent[0]; }
    int32_t materialId() const { return _materialId; }
    const vec3 toWorld(const vec3& surface) const { return _tangent * surface; }
    const vec3 toSurface(const vec3& world) const { return world * _tangent; }

    const bool is_camera() const { return _materialId == 0; }
    const bool is_light() const { return _materialId < 0 && _materialId > INT32_MIN; }
    const bool is_solid() const { return _materialId > 0; }
    const bool is_present() const { return _materialId != INT32_MIN; }
};

}
