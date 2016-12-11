#pragma once
#include <RayIsect.hpp>
#include <SurfacePoint.hpp>
#include <atomic>
#include <glm>

namespace haste {

class Intersector {
 public:
  virtual ~Intersector();

  virtual float occluded(const SurfacePoint& origin,
                         const SurfacePoint& target) const = 0;

  virtual SurfacePoint intersect(const SurfacePoint& surface, vec3 direction,
                                 float tfar) const = 0;

  SurfacePoint intersect(const SurfacePoint& surface, vec3 direction) const;

  SurfacePoint intersectMesh(const SurfacePoint& origin, vec3 direction,
                             float tfar) const;

  SurfacePoint intersectMesh(const SurfacePoint& origin, vec3 direction) const;
};
}
