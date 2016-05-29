#include <VertexMerging.hpp>

namespace haste {

VCM::VCM(size_t numPhotons, size_t numGather, float maxRadius)
    : _numPhotons(numPhotons)
    , _numGather(numGather)
    , _maxRadius(maxRadius) { }

void VCM::preprocess(
    const shared<const Scene>& scene,
    RandomEngine& engine,
    const function<void(string, float)>& progress,
    bool parallel)
{
    Technique::preprocess(scene, engine, progress, parallel);

    _traceLightPaths(scene.get(), engine);
}

void VCM::render(
    ImageView& view,
    RandomEngine& engine,
    size_t cameraId)
{
    auto trace = [&](RandomEngine& engine, Ray ray) -> vec3 {
        return vec3(1.0f, 0.0f, 1.0f);
    };

    for_each_ray(view, engine, _scene->cameras(), cameraId, trace);
}

string VCM::name() const {
    return "Vertex Merging";
}

void VCM::_traceLightPaths(const Scene* scene, RandomEngine& engine)
{





}

}
