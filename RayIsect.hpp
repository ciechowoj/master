#pragma once
#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>
#include <Geometry.hpp>

namespace haste {

struct RayIsect : public RTCRay {
private:
    using cpvec3 = const vec3*;

public:
    const bool isPresent() const { return geomID != RTC_INVALID_GEOMETRY_ID; }
    const bool isLight() const { return geomID == 0; }
    const bool isMesh() const { return geomID > 0 && isPresent(); }

    const size_t meshId() const { return geomID - 1; }
    const size_t faceId() const { return primID; }
    const size_t primId() const { return primID; }

    const vec3 normal() const { return normalize(-(const vec3&)Ng); }
    const vec3 omega() const { return normalize(-(const vec3&)dir); }
    const vec3 position() const { return *cpvec3(org) + *cpvec3(dir) * tfar; }
};

const unsigned newMesh(RTCScene scene, const Geometry& geometry);

}
