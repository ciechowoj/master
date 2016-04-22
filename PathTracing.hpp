#pragma once
#include <Technique.hpp>

namespace haste {

class PathTracing : public Technique {
public:
    PathTracing();

    void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId) override;

    vec3 trace(RandomEngine& engine, Ray ray);

    string name() const override;
};

}
