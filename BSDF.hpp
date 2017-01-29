#pragma once
#include <Intersector.hpp>
#include <Sample.hpp>

namespace haste {

struct SurfacePoint;

struct BSDFQuery {
  vec3 throughput = vec3(0.0f);
  float density = 0.0f;
  float densityRev = 0.0f;
  float specular = 0.0f;
  float glossiness = 0.0f;

  BSDFQuery reverse() const;
};

struct BSDFSample : BSDFQuery {
  vec3 omega = vec3(0.0f);
};

struct BSDFBoundedSample {
  vec3 omega;
  float adjust;
};

class BSDF {
 public:
  BSDF();

  virtual ~BSDF();

  virtual BSDFQuery query(const SurfacePoint& surface, vec3 incident,
                          vec3 outgoing) const = 0;

  virtual BSDFSample sample(random_generator_t& engine,
                            const SurfacePoint& point, vec3 omega) const = 0;

  virtual BSDFBoundedSample sample_bounded(random_generator_t& generator,
                                           const SurfacePoint& surface,
                                           bounding_sphere_t target,
                                           vec3 omega) const;

  virtual float gathering_density(random_generator_t& generator,
                                  const Intersector* Intersector,
                                  const SurfacePoint& surface,
                                  bounding_sphere_t target, vec3 omega) const;

  virtual uint32_t light_id() const;

  BSDF(const BSDF&) = delete;
  BSDF& operator=(const BSDF&) = delete;
};

class LightBSDF : public BSDF {
 public:
  LightBSDF(bounding_sphere_t sphere, uint32_t light_id);
  BSDFSample sample(random_generator_t& generator, const SurfacePoint& surface,
                    vec3 omega) const override;
  BSDFQuery query(const SurfacePoint& surface, vec3 incident,
                  vec3 outgoing) const override;

  BSDFBoundedSample sample_bounded(random_generator_t& generator,
                                   const SurfacePoint& surface,
                                   bounding_sphere_t target,
                                   vec3 omega) const override;

  uint32_t light_id() const override;

 private:
  bounding_sphere_t _sphere;
  uint32_t _light_id;
};

class sun_light_bsdf : public BSDF {
 public:
  sun_light_bsdf(bounding_sphere_t sphere, uint32_t light_id);
  BSDFSample sample(random_generator_t& generator, const SurfacePoint& surface,
                    vec3 omega) const override;
  BSDFQuery query(const SurfacePoint& surface, vec3 incident,
                  vec3 outgoing) const override;

  BSDFBoundedSample sample_bounded(random_generator_t& generator,
                                   const SurfacePoint& surface,
                                   bounding_sphere_t target,
                                   vec3 omega) const override;

  uint32_t light_id() const override;

 private:
  bounding_sphere_t _sphere;
  uint32_t _light_id;
};

class CameraBSDF : public BSDF {
 public:
  CameraBSDF();

  BSDFQuery query(const SurfacePoint& surface, vec3 incident,
                  vec3 outgoing) const override;
  BSDFSample sample(random_generator_t& generator, const SurfacePoint& surface,
                    vec3 omega) const override;
  BSDFBoundedSample sample_bounded(random_generator_t& generator,
                                   const SurfacePoint& surface,
                                   bounding_sphere_t target,
                                   vec3 omega) const override;
};

class DiffuseBSDF : public BSDF {
 public:
  DiffuseBSDF(vec3 diffuse);

  BSDFQuery query(const SurfacePoint& surface, vec3 incident,
                  vec3 outgoing) const override;

  BSDFSample sample(random_generator_t& generator, const SurfacePoint& surface,
                    vec3 omega) const override;

  BSDFBoundedSample sample_bounded(random_generator_t& generator,
                                   const SurfacePoint& surface,
                                   bounding_sphere_t target,
                                   vec3 omega) const override;

 private:
  BSDFQuery _query(vec3 gnormal, vec3 incident, vec3 outgoing) const;

  vec3 _diffuse;
};

class PhongBSDF : public BSDF {
 public:
  PhongBSDF(vec3 diffuse, vec3 specular, float power);

  BSDFQuery query(const SurfacePoint& surface, vec3 incident,
                  vec3 outgoing) const override;

  BSDFSample sample(random_generator_t& generator, const SurfacePoint& surface,
                    vec3 omega) const override;

  BSDFBoundedSample sample_bounded(random_generator_t& generator,
                                   const SurfacePoint& surface,
                                   bounding_sphere_t target,
                                   vec3 omega) const override;

 private:
  BSDFQuery _query(vec3 incident, vec3 outgoing, float same_side) const;

  vec3 _diffuse;
  vec3 _specular;
  float _power;
  float _diffuse_probability;
};

class DeltaBSDF : public BSDF {
 public:
  DeltaBSDF();

  BSDFQuery query(const SurfacePoint& surface, vec3 incident,
                  vec3 outgoing) const override;
};

class ReflectionBSDF : public DeltaBSDF {
 public:
  BSDFSample sample(random_generator_t& generator, const SurfacePoint& surface,
                    vec3 omega) const override;
};

class TransmissionBSDF : public DeltaBSDF {
 public:
  TransmissionBSDF(float internalIOR, float externalIOR);

  BSDFSample sample(random_generator_t& generator, const SurfacePoint& surface,
                    vec3 omega) const override;

 private:
  float externalOverInternalIOR;
  float internalIOR;
};
}
