#include <runtime_assert>
#include <GLFW/glfw3.h>
#include <Technique.hpp>

namespace haste {

Technique::Technique() { }

Technique::~Technique() { }

void Technique::preprocess(
    const shared<const Scene>& scene,
    RandomEngine& engine,
    const function<void(string, float)>& progress,
    size_t numThreads)
{
    _numNormalRays = 0;
    _numShadowRays = 0;
    _numSamples = 0;
    _scene = scene;
}

void Technique::render(
    ImageView& view,
    RandomEngine& engine,
    size_t cameraId,
    size_t numThreads)
{
    size_t numNormalRays = _scene->numNormalRays();
    size_t numShadowRays = _scene->numShadowRays();

    render(view, engine, cameraId);

    _numNormalRays += _scene->numNormalRays() - numNormalRays;
    _numShadowRays += _scene->numShadowRays() - numShadowRays;
    _numSamples = size_t(view.last().w);
}

}
