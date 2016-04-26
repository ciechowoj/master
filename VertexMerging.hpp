#pragma once
#include <Technique.hpp>

namespace haste {

class VertexMerging : public Technique {
public:
    VertexMerging();

     void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId) override;

    string name() const override;
};

}
