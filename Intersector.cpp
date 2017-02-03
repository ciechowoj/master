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

}
