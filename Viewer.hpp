#pragma once
#include <Technique.hpp>

namespace haste {

class Viewer : public Technique {
public:
    Viewer(
        const vector<dvec4>& data,
        size_t width,
        size_t height);

    double render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId) override;

    string name() const override;
    string id() const override;

private:
    vector<dvec4> _data;
    size_t _width;
    size_t _height;
};

}
