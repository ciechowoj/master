#pragma once
#include <Technique.hpp>

namespace haste {

class BidirectionalPathTracing : public Technique {
public:
    BidirectionalPathTracing();

     void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId) override;

    string name() const override;

private:
    vec3 _pathtrace(RandomEngine& engine, Ray ray);

    double _progress;
};

}
