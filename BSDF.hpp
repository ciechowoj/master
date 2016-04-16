#pragma once
#include <utility.hpp>

namespace haste {

struct SurfacePoint;

struct BSDFSample {
    vec3 throughput;
    vec3 direction;
    bool specular;
};

class BSDF {
public:
    BSDFSample sample(
        const vec3& incident) const;

    BSDFSample sample(
        const mat3& lightToWorld,
        const mat3& worldToLight,
        const vec3& incident) const;

    vec3 eval(
        const vec3& incident,
        const vec3& reflected) const;

    static BSDF lambert(const vec3& diffuse);

    BSDFSample sample(
        const SurfacePoint& point,
        const vec3& reflected) const;

    vec3 query(
        const SurfacePoint& point,
        const vec3& incident,
        const vec3& reflected) const;
private:
    mutable HemisphereSampler sampler;
    vec3 diffuse;
};

}


