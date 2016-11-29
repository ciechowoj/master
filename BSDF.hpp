#pragma once
#include <Sample.hpp>

namespace haste {

struct SurfacePoint;

struct BSDFQuery {
    vec3 throughput = vec3(0.0f);
    float density = 0.0f;
    float densityRev = 0.0f;
    float specular = 0.0f;
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

    const BSDFQuery query(
        const SurfacePoint& point,
        const vec3& incident,
        const vec3& outgoing) const;

    virtual const BSDFSample sample(
        RandomEngine& engine,
        const vec3& omega) const = 0;

    virtual const BSDFBoundedSample sampleBounded(
        RandomEngine& engine,
        const vec3& omega,
        const angular_bound_t& bound) const;

    const BSDFSample sample(
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
};

class LightBSDF : public BSDF {
public:
    LightBSDF(vec3 radiance);

    const BSDFSample sample(
        RandomEngine& engine,
        const vec3& omega) const override;

    const BSDFQuery query(
        const vec3& incident,
        const vec3& outgoing) const override;

private:
    vec3 _radiance;
};

class CameraBSDF : public BSDF {
public:
    const BSDFSample sample(
        RandomEngine& engine,
        const vec3& omega) const override;

    const BSDFQuery query(
        const vec3& incident,
        const vec3& outgoing) const override;

};

}
