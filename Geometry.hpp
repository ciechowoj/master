#pragma once
#include <glm>

namespace haste {

class Geometry {
public:
    virtual ~Geometry();

    virtual const bool usesQuads() const;
    virtual const bool usesTriangles() const;
    virtual const size_t numQuads() const;
    virtual const size_t numTriangles() const;
    virtual const size_t numIndices() const;
    virtual const size_t numVertices() const;
    virtual void updateBuffers(int* indices, vec4* vertices) const = 0;
};

}
