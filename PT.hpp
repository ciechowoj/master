#pragma once
#include <Technique.hpp>

namespace haste {

class PathTracing : public Technique {
public:
    PathTracing(size_t minSubpath, float roulette, size_t num_threads);

    vec3 _traceEye(render_context_t& context, Ray ray) override;

    string name() const override;

private:
    const size_t _minLength;
    const float _roulette;
};

}
