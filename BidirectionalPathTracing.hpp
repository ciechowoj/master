#pragma once
#include <Technique.hpp>

namespace haste {

class BidirectionalPathTracing : public Technique {
public:
    BidirectionalPathTracing();

    void updateInteractive(
        size_t width,
        size_t height,
        vec4* image,
        double timeQuantum) override;

    string stageName() const override;
    double stageProgress() const override;

private:
    vec3 _pathtrace(RandomEngine& engine, Ray ray);

    double _progress;
};

}
