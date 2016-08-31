#pragma once
#include <Technique.hpp>

namespace haste {

template <int C> class FixedBeta {
public:
    float beta(float x);
    string name() const;
};

class VariableBeta {
public:
    float beta(float x);
    string name() const;
    void init(float beta);
private:
    float _beta = 1.0f;
};


template <class Beta> class MBPT : public Technique, protected Beta {
public:
    MBPT(size_t minSubpath = 3, float roulette = 0.5f);

    string name() const override;

private:
    struct LightVertex {
        SurfacePoint surface;
        vec3 _omega;
        vec3 throughput;
        float specular;
        float a, A;

        const vec3& position() const { return surface.position(); }
        const vec3& normal() const { return surface.normal(); }
        const vec3& gnormal() const { return surface.gnormal(); }
        const vec3& omega() const { return _omega; }
    };

    struct EyeVertex {
        SurfacePoint surface;
        vec3 _omega;
        vec3 throughput;
        float specular;
        float c, C;

        const vec3& position() const { return surface.position(); }
        const vec3& normal() const { return surface.normal(); }
        const vec3& gnormal() const { return surface.gnormal(); }
        const vec3& omega() const { return _omega; }
    };

    static const size_t _maxSubpath = 1024;
    const size_t _minSubpath;
    const float _roulette;

    vec3 _trace(RandomEngine& engine, const Ray& ray);
    void _trace(RandomEngine& engine, size_t& size, LightVertex* path);
    vec3 _connect0(RandomEngine& engine, const EyeVertex& eye);
    vec3 _connect1(RandomEngine& engine, const EyeVertex& eye);
    vec3 _connect(const EyeVertex& eye, const LightVertex& light);
    vec3 _connect(RandomEngine& engine, const EyeVertex& eye, size_t size, const LightVertex* path);
};

typedef MBPT<FixedBeta<0>> BPT0;
typedef MBPT<FixedBeta<1>> BPT1;
typedef MBPT<FixedBeta<2>> BPT2;

class BPTb : public MBPT<VariableBeta> {
public:
    BPTb(size_t minSubpath = 3, float roulette = 0.5f, float beta = 1.0f);
};

}
