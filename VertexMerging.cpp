#include <VertexMerging.hpp>

namespace haste {

VertexMerging::VertexMerging() { }

 void VertexMerging::render(
    ImageView& view,
    RandomEngine& engine,
    size_t cameraId)
 {
    auto trace = [&](RandomEngine& engine, Ray ray) -> vec3 {
        return vec3(1.0f, 0.0f, 1.0f);
    };

    for_each_ray(view, engine, _scene->cameras(), cameraId, trace);
 }

string VertexMerging::name() const {
    return "Vertex Merging";
}

}
