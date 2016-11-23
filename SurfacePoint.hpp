#pragma once
#include <cstdint>

namespace haste {

using std::uint32_t;

struct SurfacePoint {
    vec3 _position;
    mat3 _tangent;
    uint32_t _materialId;

    SurfacePoint() = default;

    SurfacePoint(
        const vec3& position,
        const vec3& normal,
        const vec3& tangent,
        uint32_t materialId = 0)
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
    const vec3& gnormal() const { return _tangent[1]; }
    uint32_t materialId() const { return _materialId; }
    const vec3 toWorld(const vec3& surface) const { return _tangent * surface; }
    const vec3 toSurface(const vec3& world) const { return world * _tangent; }
};

inline SurfacePoint camera_surface(const mat4& world_to_view_mat4) {
    return SurfacePoint();
}


}
