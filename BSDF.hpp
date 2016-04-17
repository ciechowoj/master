#pragma once
#include <Sample.hpp>

namespace haste {

struct SurfacePoint;

struct BSDFSample {
    vec3 _throughput;
    vec3 _omega;
    float _density;
    float _densityInv;
    bool _specular;

    const vec3& throughput() const { return _throughput; }
    const vec3& omega() const { return _omega; }
    const float density() const { return _density; }
    const float densityInv() const { return _densityInv; }
    const bool specular() const { return _specular; }
};

class BSDF {
public:
    BSDF();
    virtual ~BSDF();

    virtual const vec3 query(
        const vec3& a,
        const vec3& b) const = 0;

    const vec3 query(
        const SurfacePoint& point,
        const vec3& a,
        const vec3& b) const;

    virtual const BSDFSample sample(
        RandomEngine& source,
        const vec3& omega) const = 0;

    const BSDFSample sample(
        RandomEngine& source,
        const SurfacePoint& point,
        const vec3& omega) const;

    virtual BSDFSample scatter(
        RandomEngine& source,
        const SurfacePoint& point,
        const vec3& omega) const = 0;

    BSDF(const BSDF&) = delete;
    BSDF& operator=(const BSDF&) = delete;
};

class DiffuseBSDF : public BSDF {
public:
    DiffuseBSDF(const vec3& diffuse);

    const vec3 query(
        const vec3& a,
        const vec3& b) const override;

    const BSDFSample sample(
        RandomEngine& source,
        const vec3& omega) const override;

    BSDFSample scatter(
        RandomEngine& source,
        const SurfacePoint& point,
        const vec3& omega) const override;

private:
    vec3 _diffuse;
};

}
