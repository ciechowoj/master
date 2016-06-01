#include <Intersector.hpp>

namespace haste {

Intersector::~Intersector()
{ }

const RayIsect Intersector::intersectMesh(
    const vec3& origin,
    const vec3& direction) const
{
	RayIsect isect = intersect(origin, direction);

	while (isect.isLight())
		isect = intersect(isect.position(), direction);

	return isect;
}

}
