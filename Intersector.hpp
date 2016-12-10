#pragma once
#include <RayIsect.hpp>
#include <SurfacePoint.hpp>
#include <atomic>
#include <glm>

namespace haste {

class Intersector {
 public:
  virtual ~Intersector();

  virtual const RayIsect intersect(const vec3& origin,
                                   const vec3& direction) const = 0;

  virtual const RayIsect intersect(const vec3& origin, const vec3& direction,
                                   float bound) const = 0;

  virtual const float occluded(const vec3& origin,
                               const vec3& target) const = 0;

  virtual const RayIsect intersectLight(const vec3& origin,
                                        const vec3& direction) const = 0;

  virtual const RayIsect intersectMesh(const vec3& origin,
                                       const vec3& direction) const;

  virtual const RayIsect intersectMesh(const vec3& origin,
                                       const vec3& direction,
                                       float bound) const;

  virtual SurfacePoint intersect(const SurfacePoint& surface, vec3 direction,
                                 float tfar) const = 0;

  SurfacePoint intersect(const SurfacePoint& surface, vec3 direction) const;

  SurfacePoint intersectMesh(const SurfacePoint& origin, vec3 direction,
                             float tfar) const;

  SurfacePoint intersectMesh(const SurfacePoint& origin, vec3 direction) const;
};
}
