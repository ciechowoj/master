#pragma once
#include <cstdint>

namespace haste {

using std::uint32_t;

enum class entity_type : uint32_t {
    camera = 0,
    mesh = 1,
    light = 2,
    empty = 3
};

inline uint32_t encode_material(uint32_t material_id, entity_type type) {
    return material_id << 2 | uint32_t(type);
}

inline uint32_t decode_material(uint32_t material_id) {
    return material_id >> 2;
}

struct SurfacePoint {
    vec3 _position;
    vec3 gnormal;
    mat3 _tangent;
    uint32_t material_id = UINT32_MAX;

    SurfacePoint() = default;

    SurfacePoint(
        const vec3& position,
        const vec3& normal,
        const vec3& tangent,
        uint32_t material_id)
    {
        _position = position;
        _tangent[0] = normalize(cross(normal, tangent));
        _tangent[1] = normal;
        _tangent[2] = tangent;
        material_id = material_id;
    }

    const vec3& position() const { return _position; }
    const vec3& normal() const { return _tangent[1]; }
    const vec3& tangent() const { return _tangent[2]; }
    const vec3& bitangent() const { return _tangent[0]; }
    const vec3 toWorld(const vec3& surface) const { return _tangent * surface; }
    const vec3 toSurface(const vec3& world) const { return world * _tangent; }

    uint32_t material_index() const { return material_id >> 2; }

    bool is_camera() const { return (material_id & 3u) == uint32_t(entity_type::camera); }
    bool is_mesh() const { return (material_id & 3u) == uint32_t(entity_type::mesh); }
    bool is_light() const { return (material_id & 3u) == uint32_t(entity_type::light); }
    bool is_present() const { return material_id != UINT32_MAX; }
};

}
