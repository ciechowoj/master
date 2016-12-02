#pragma once
#include <glm>
#include <type_traits>

namespace haste {

template <class T> struct has_surface {
private:
    template<class U, class R, R U::*> struct _sfinae {};
    template<class U> static char _test(_sfinae<U, decltype(U::surface), &U::surface>*);
    template<class U> static int _test(...);
public:
    static const bool value = sizeof(_test<T>(0)) == sizeof(char);
};

struct Edge
{
private:
    template <class A, class B>
    void _init(const A& fst, const B& snd, std::true_type) {
        distSqInv = 1.0f / distance2(fst.surface.position(), snd.surface.position());
        fCosTheta = abs(dot(snd.omega, snd.surface.normal()));
        bCosTheta = abs(dot(snd.omega, fst.surface.normal()));
        fGeometry = distSqInv * fCosTheta;
        bGeometry = distSqInv * bCosTheta;
    }

    template <class A, class B>
    void _init(const A& fst, const B& snd, const vec3& omega, std::false_type, std::true_type) {
        distSqInv = 1.0f / distance2(fst.position(), snd.surface.position());
        fCosTheta = abs(dot(omega, snd.surface.normal()));
        bCosTheta = abs(dot(omega, fst.gnormal()));
        fGeometry = distSqInv * fCosTheta;
        bGeometry = distSqInv * bCosTheta;
    }

    template <class A, class B>
    void _init(const A& fst, const B& snd, const vec3& omega, std::true_type, std::true_type) {
        distSqInv = 1.0f / distance2(fst.surface.position(), snd.surface.position());
        fCosTheta = abs(dot(omega, snd.surface.normal()));
        bCosTheta = abs(dot(omega, fst.surface.normal()));
        fGeometry = distSqInv * fCosTheta;
        bGeometry = distSqInv * bCosTheta;
    }

public:

    template <class A, class B>
    Edge(const A& fst, const B& snd) {
        _init(fst, snd, std::integral_constant<bool, has_surface<A>::value && has_surface<B>::value>());
    }

    template <class A, class B> Edge(const A& fst, const B& snd, const vec3& omega) {
        _init(
            fst,
            snd,
            omega,
            std::integral_constant<bool, has_surface<A>::value>(),
            std::integral_constant<bool, has_surface<B>::value>());
    }

    float distSqInv;
    float fCosTheta;
    float bCosTheta;
    float fGeometry;
    float bGeometry;
};

}
