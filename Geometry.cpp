#include <Geometry.hpp>

namespace haste {

Geometry::~Geometry() { }

const bool Geometry::usesQuads() const {
    return false;
}

const bool Geometry::usesTriangles() const {
    return false;
}

const size_t Geometry::numQuads() const {
    return 0;
}

const size_t Geometry::numTriangles() const {
    return 0;
}

const size_t Geometry::numIndices() const {
	return usesTriangles() ? numTriangles() * 3 : numQuads() * 4;
}

const size_t Geometry::numVertices() const {
	return usesTriangles() ? numTriangles() * 3 : numQuads() * 4;
}

}
