#pragma once
#include <RayIsect.hpp>
#include <Sample.hpp>
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

  virtual SurfacePoint intersectMesh(const SurfacePoint& origin, vec3 direction,
                                     float tfar) const = 0;

  SurfacePoint intersectMesh(const SurfacePoint& origin, vec3 direction) const;

  bool intersectFast(const SurfacePoint& surface, const vec3& direction,
                     const bounding_sphere_t& surface_target,
                     const bounding_sphere_t& world_target) const;

 protected:
  mutable std::atomic<size_t> _numIntersectRays;
  mutable std::atomic<size_t> _numOccludedRays;

  mutable RTCScene rtcScene;
};
}
