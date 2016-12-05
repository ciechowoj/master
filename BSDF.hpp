#pragma once
#include <Sample.hpp>
#include <Intersector.hpp>

namespace haste {

struct SurfacePoint;

struct BSDFQuery {
    vec3 throughput = vec3(0.0f);
    float density = 0.0f;
    float densityRev = 0.0f;
    float specular = 0.0f;
};

struct BSDFSample {
    vec3 omega = vec3(0.0f);
    vec3 throughput = vec3(0.0f);
    float density = 0.0f;
    float densityRev = 0.0f;
    float specular = 0.0f;
};

struct BSDFBoundedSample {
    vec3 omega;
    float adjust;
};

class BSDF {
public:
    BSDF();

    virtual ~BSDF();

    virtual const BSDFQuery query(vec3 incident, vec3 outgoing) const;

    const BSDFQuery query(
        const SurfacePoint& point,
        vec3 incident,
        vec3 outgoing) const;

    virtual const BSDFSample sample(
        RandomEngine& engine,
        vec3 omega) const = 0;

    const BSDFSample sample(
        RandomEngine& engine,
        const SurfacePoint& point,
        vec3 omega) const;

    virtual BSDFBoundedSample sample_bounded(
        random_generator_t& generator,
        bounding_sphere_t target,
        vec3 omega) const;

    BSDF(const BSDF&) = delete;
    BSDF& operator=(const BSDF&) = delete;
};

class DeltaBSDF : public BSDF {
public:
    const BSDFQuery query(vec3 incident, vec3 outgoing) const override;
};

class LightBSDF : public BSDF {
public:
    LightBSDF(vec3 radiance);

    const BSDFSample sample(
        RandomEngine& engine,
        vec3 omega) const override;

    const BSDFQuery query(vec3 incident, vec3 outgoing) const override;

private:
    vec3 _radiance;
};

class CameraBSDF : public BSDF {
public:
    const BSDFSample sample(RandomEngine& engine, vec3 omega) const override;
    const BSDFQuery query(vec3 incident, vec3 outgoing) const override;

};

}
