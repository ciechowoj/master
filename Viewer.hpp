#pragma once
#include <Technique.hpp>

namespace haste {

class Viewer : public Technique {
public:
    Viewer(
        const vector<dvec4>& data,
        size_t width,
        size_t height);

    void render(
        subimage_view_t& view,
        RandomEngine& engine,
        size_t cameraId,
        const vector<vec3>& reference,
        const vector<ivec3>& trace_points) override;

private:
    vector<dvec4> _data;
    size_t _width;
    size_t _height;
};

}
