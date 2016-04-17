#pragma once
#include <Technique.hpp>

namespace haste {

class BFDirectLighting : public Technique {
public:
    BFDirectLighting();

    void updateInteractive(
    	size_t width,
        size_t height,
        vec4* image,
        double timeQuantum) override;

    string stageName() const override;
    double stageProgress() const override;

private:
    vec3 _trace(RandomEngine& source, Ray ray);

    double _progress;
};

}
