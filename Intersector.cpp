#include <Intersector.hpp>

namespace haste {

Intersector::~Intersector() {}

SurfacePoint Intersector::intersect(const SurfacePoint& surface,
                                    vec3 direction) const {
  return intersect(surface, direction, INFINITY);
}

SurfacePoint Intersector::intersectMesh(const SurfacePoint& origin,
                                        vec3 direction) const {
  return intersectMesh(origin, direction, INFINITY);
}

bool Intersector::intersectFast(const SurfacePoint& surface,
                                const vec3& direction,
                                const bounding_sphere_t& surface_target,
                                const bounding_sphere_t& world_target) const {
  float target_length = length(surface_target.center);
  float r_sq = world_target.radius * world_target.radius;

  RayIsect rtcRay;
  (*(vec3*)rtcRay.org) =
      surface.position() +
      (dot(surface.gnormal, direction) > 0.0f ? 1.0f : -1.0f) *
          surface.gnormal * 0.0001f;

  (*(vec3*)rtcRay.dir) = direction;
  rtcRay.tnear = 0.0f;
  rtcRay.tfar = target_length + world_target.radius;
  rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
  rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
  rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
  rtcRay.mask = 1u << uint32_t(entity_type::mesh);
  rtcRay.time = 0.f;
  rtcIntersect(rtcScene, rtcRay);

  ++_numIntersectRays;

  vec3 isect = (vec3&)rtcRay.org + (vec3&)rtcRay.dir * rtcRay.tfar;
  float d_sq = distance2(world_target.center, isect);

  return rtcRay.geomID != RTC_INVALID_GEOMETRY_ID && d_sq < r_sq;
}

}
