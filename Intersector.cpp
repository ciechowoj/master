#include <Intersector.hpp>

namespace haste {

Intersector::~Intersector() {}

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
	return intersectMesh(origin, direction, INFINITY);
}

}
