#pragma once
#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>
#include <glm>

namespace haste {

struct RayIsect : public RTCRay {
private:
    using cpvec3 = const vec3*;

public:
    const bool isPresent() const { return geomID != RTC_INVALID_GEOMETRY_ID; }
    const bool isLight() const { return geomID == 0; }
    const bool isMesh() const { return geomID > 0; }

    const size_t meshId() const { return geomID - 1; }
    const size_t faceId() const { return primID; }

    vec3 gnormal() const {
        return normalize(vec3(-Ng[0], -Ng[1], -Ng[2]));
    }

    vec3 position() const {
        return *cpvec3(org) + *cpvec3(dir) * tfar;
    }

    const vec3 incident() const {
        return -normalize(*cpvec3(dir));
    }
};

}
