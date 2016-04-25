#pragma once

namespace haste {

struct SurfacePoint {
    vec3 _position;
    vec3 _gnormal;
    mat3 toWorldM;
    size_t materialID;


    const vec3& position() const { return _position; }

    const vec3& normal() const {
        return toWorldM[1];
    }

    const vec3& gnormal() const {
        return _gnormal;
    }

    const vec3& tangent() const {
        return toWorldM[2];
    }

    const vec3& bitangent() const {
        return toWorldM[0];
    }

    const vec3 toWorld(const vec3& v) const {
        return toWorldM * v;
    }

    const vec3 toSurface(const vec3& world) const {
        return world * toWorldM;
    }
};

}
