#pragma once
#include <Technique.hpp>

namespace haste {

class Viewer : public Technique {
public:
    Viewer(
        const vector<vec4>& data,
        size_t width,
        size_t height);

    void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId,
        bool parallel) override;

    string name() const override;

private:
    vector<vec4> _data;
    size_t _width;
    size_t _height;
};

}