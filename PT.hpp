#pragma once
#include <Technique.hpp>

namespace haste {

class PathTracing : public Technique {
public:
    PathTracing(size_t minSubpath, float roulette);

    void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId) override;

    vec3 trace(RandomEngine& engine, Ray ray);
    vec3 trace(RandomEngine& engine, Ray ray, const Scene& scene);

    string name() const override;

private:
    const size_t _minLength;
    const float _roulette;
};

}
