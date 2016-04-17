#pragma once

namespace haste {

struct SurfacePoint {
    vec3 position;
    mat3 toWorldM;
    size_t materialID;

    const vec3& normal() const {
        return toWorldM[1];
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
