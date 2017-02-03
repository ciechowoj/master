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

vec4 Intersector::intersectFast(const SurfacePoint& surface, vec3 direction,
                           float tfar) const {
  RayIsect rtcRay;
  (*(vec3*)rtcRay.org) =
      surface.position() +
      (dot(surface.gnormal, direction) > 0.0f ? 1.0f : -1.0f) *
          surface.gnormal * 0.0001f;

  (*(vec3*)rtcRay.dir) = direction;
  rtcRay.tnear = 0.0f;
  rtcRay.tfar = tfar;
  rtcRay.geomID = RTC_INVALID_GEOMETRY_ID;
  rtcRay.primID = RTC_INVALID_GEOMETRY_ID;
  rtcRay.instID = RTC_INVALID_GEOMETRY_ID;
  rtcRay.mask = 1u << uint32_t(entity_type::mesh);
  rtcRay.time = 0.f;
  rtcIntersect(rtcScene, rtcRay);

  ++_numIntersectRays;

  return vec4((vec3&)rtcRay.org + (vec3&)rtcRay.dir * rtcRay.tfar,
              rtcRay.geomID != RTC_INVALID_GEOMETRY_ID ? 1.0f : 0.0f);
}

}
