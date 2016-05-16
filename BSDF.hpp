#pragma once
#include <Sample.hpp>

namespace haste {

struct SurfacePoint;

struct BSDFSample {
    vec3 _throughput;
    vec3 _omega;
    float _density;
    float _densityInv;
    float _specular;

    const vec3& throughput() const { return _throughput; }
    const vec3& omega() const { return _omega; }
    const float density() const { return _density; }
    const float densityInv() const { return _densityInv; }
    const float specular() const { return _specular; }
    const bool zero() const;
};

class BSDF {
public:
    BSDF();
    virtual ~BSDF();

    virtual const vec3 query(
        const vec3& incomin,
        const vec3& b,
        const vec3& n) const = 0;

    const vec3 query(
        const SurfacePoint& point,
        const vec3& a,
        const vec3& b,
        const vec3& n) const;

    virtual const float density(
        const vec3& incident,
        const vec3& reflected,
        const vec3& normal) const;

    const float density(
        const SurfacePoint& point,
        const vec3& incident,
        const vec3& reflected) const;

    virtual const BSDFSample sample(
        RandomEngine& engine,
        const vec3& omega) const = 0;

    const BSDFSample sample(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omega) const;

    virtual BSDFSample scatter(
        RandomEngine& engine,
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
        const vec3& b,
        const vec3& n) const override;

    const float density(
        const vec3& incident,
        const vec3& reflected,
        const vec3& normal) const override;

    const BSDFSample sample(
        RandomEngine& engine,
        const vec3& omega) const override;

    BSDFSample scatter(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omega) const override;

private:
    vec3 _diffuse;
};

class PerfectReflectionBSDF : public BSDF {
public:
    const vec3 query(
        const vec3& incident,
        const vec3& reflected,
        const vec3& normal) const override;

    const float density(
        const vec3& incident,
        const vec3& reflected,
        const vec3& normal) const override;

    const BSDFSample sample(
        RandomEngine& engine,
        const vec3& reflected) const override;

    BSDFSample scatter(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& incident) const override;
};

class PerfectTransmissionBSDF : public BSDF {
public:
    const vec3 query(
        const vec3& incident,
        const vec3& reflected,
        const vec3& normal) const override;

    const float density(
        const vec3& incident,
        const vec3& reflected,
        const vec3& normal) const override;

    const BSDFSample sample(
        RandomEngine& engine,
        const vec3& reflected) const override;

    BSDFSample scatter(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& incident) const override;
};

}
