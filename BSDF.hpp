#pragma once
#include <Sample.hpp>

namespace haste {

struct angular_bound_t {
    float theta_inf, theta_sup;
    float phi_inf, phi_sup;
};

angular_bound_t angular_bound(vec3 center, float radius);

struct SurfacePoint;

struct BSDFQuery {
    vec3 _throughput;
    float _density;
    float _densityRev;

    const vec3& throughput() const { return _throughput; }
    const float density() const { return _density; }
    const float densityRev() const { return _densityRev; }
    const float specular() const { return 1.0f - ceil(_density); }
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

struct BSDFBoundedSample {
    vec3 _omega;
    float _area;

    const vec3& omega() const { return _omega; }
    float area() const { return _area; }
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

    /* virtual const BSDFBoundedSample sampleBounded(
        RandomEngine& engine,
        const vec3& omega,
        const vec3& target,
        float radius) const; */

    virtual const BSDFSample sampleAdjoint(
        RandomEngine& engine,
        const vec3& omega) const;

    const BSDFSample sample(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omega) const;

    /* const BSDFBoundedSample sampleBounded(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omega,
        const vec3& target,
        float radius) const; */

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
