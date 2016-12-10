#include <Intersector.hpp>

namespace haste {

Intersector::~Intersector() {}

const RayIsect Intersector::intersectMesh(const vec3& origin,
                                          const vec3& direction) const {
  RayIsect isect = intersect(origin, direction);

  while (isect.isLight()) {
    isect = intersect(isect.position(), direction);
  }

  return isect;
}

const RayIsect Intersector::intersectMesh(const vec3& origin,
                                          const vec3& direction,
                                          float bound) const {
  RayIsect isect = intersect(origin, direction, bound);

  while (isect.isLight()) {
    isect = intersect(isect.position(), direction, bound);
  }

  return isect;
}

SurfacePoint Intersector::intersect(const SurfacePoint& surface,
                                    vec3 direction) const {
  return intersect(surface, direction, INFINITY);
}

SurfacePoint Intersector::intersectMesh(const SurfacePoint& origin,
                                        vec3 direction, float tfar) const {
	SurfacePoint surface = intersect(origin, direction, tfar);

  while (surface.is_light()) {
    surface = intersect(surface, direction, tfar);
  }

  return surface;
}

SurfacePoint Intersector::intersectMesh(const SurfacePoint& origin,
                                        vec3 direction) const {
	return Intersector::intersectMesh(origin, direction, INFINITY);
}

}
