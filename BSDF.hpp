#pragma once
#include <Sample.hpp>

namespace haste {

struct SurfacePoint;

struct BSDFQuery {
    vec3 _throughput;
    float _density;
    float _densityRev;

    const vec3& throughput() const { return _throughput; }
    const float density() const { return _density; }
    const float densityRev() const { return _densityRev; }
    const bool zero() const;
};

struct BSDFSample {
    vec3 _throughput;
    vec3 _omega;
    float _density;
    float _densityRev;
    float _specular;

    const vec3& throughput() const { return _throughput; }
    const vec3& omega() const { return _omega; }
    const float density() const { return _density; }
    const float densityRev() const { return _densityRev; }
    const float specular() const { return _specular; }
    const bool zero() const;
    const BSDFQuery query() const;
};

class BSDF {
public:
    BSDF();

    virtual ~BSDF();

    virtual const BSDFQuery query(
        const vec3& incident,
        const vec3& outgoing) const;

    virtual const BSDFQuery queryAdjoint(
        const vec3& incident,
        const vec3& outgoing) const;

    const BSDFQuery query(
        const SurfacePoint& point,
        const vec3& incident,
        const vec3& outgoing) const;

    const BSDFQuery queryAdjoint(
        const SurfacePoint& point,
        const vec3& incident,
        const vec3& outgoing) const;

    virtual const BSDFSample sample(
        RandomEngine& engine,
        const vec3& omega) const = 0;

    virtual const BSDFSample sampleAdjoint(
        RandomEngine& engine,
        const vec3& omega) const;

    const BSDFSample sample(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omega) const;

    const BSDFSample sampleAdjoint(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omega) const;

    virtual BSDFSample scatter(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omega) const;

    BSDF(const BSDF&) = delete;
    BSDF& operator=(const BSDF&) = delete;
};

class DeltaBSDF : public BSDF {
public:
    virtual const BSDFQuery query(
        const vec3& incident,
        const vec3& outgoing) const;

    virtual const BSDFQuery queryAdjoint(
        const vec3& incident,
        const vec3& outgoing) const;
};

}
