#pragma once
#include <glm>
#include <atomic>
#include <RayIsect.hpp>

namespace haste {

class Intersector {
public:
    virtual ~Intersector();

    virtual const RayIsect intersect(
        const vec3& origin,
        const vec3& direction) const = 0;

    virtual const float occluded(const vec3& origin,
        const vec3& target) const = 0;

    virtual const RayIsect intersectLight(
        const vec3& origin,
        const vec3& direction) const = 0;

    virtual const RayIsect intersectMesh(
        const vec3& origin,
        const vec3& direction) const;
};

}
