#pragma once
#include <Technique.hpp>

namespace haste {

class DirectIllumination : public Technique {
public:
    DirectIllumination();

    void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId) override;

    vec3 trace(RandomEngine& source, Ray ray);

    string name() const override;
};

}
