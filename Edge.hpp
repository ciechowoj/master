#pragma once
#include <glm>

namespace haste {

struct Edge
{
    template <class A, class B> Edge(const A& fst, const B& snd)
    {
        distSqInv = 1.0f / distance2(fst.position(), snd.position());
        fCosTheta = abs(dot(fst.omega(), snd.gnormal()));
        bCosTheta = abs(dot(fst.omega(), fst.gnormal()));
        fGeometry = distSqInv * fCosTheta;
        bGeometry = distSqInv * bCosTheta;
    }

    template <class A, class B> Edge(const A& fst, const B& snd, const vec3& omega)
    {
        distSqInv = 1.0f / distance2(fst.position(), snd.position());
        fCosTheta = abs(dot(omega, snd.gnormal()));
        bCosTheta = abs(dot(omega, fst.gnormal()));
        fGeometry = distSqInv * fCosTheta;
        bGeometry = distSqInv * bCosTheta;
    }

    float distSqInv;
    float fCosTheta;
    float bCosTheta;
    float fGeometry;
    float bGeometry;
};

}
