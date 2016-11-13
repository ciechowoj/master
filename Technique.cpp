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
    bool parallel)
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
    bool parallel)
{
    size_t numNormalRays = _scene->numNormalRays();
    size_t numShadowRays = _scene->numShadowRays();

    if (parallel) {
        exec2d(_threadpool, view.xWindow(), view.yWindow(), 32,
            [&](size_t x0, size_t x1, size_t y0, size_t y1) {
            RandomEngine engine;

            ImageView subview = view;

            size_t xBegin = view._xOffset + x0;
            size_t xEnd = view._xOffset + x1;

            size_t yBegin = view._yOffset + y0;
            size_t yEnd = view._yOffset + y1;

            subview._xOffset = xBegin;
            subview._xWindow = xEnd - xBegin;

            subview._yOffset = yBegin;
            subview._yWindow = yEnd - yBegin;

            render(subview, engine, cameraId);
        });
    }
    else {
        render(view, engine, cameraId);
    }

    _numNormalRays += _scene->numNormalRays() - numNormalRays;
    _numShadowRays += _scene->numShadowRays() - numShadowRays;
    _numSamples = size_t(view.last().w);
}

void Technique::render(
    ImageView& view,
    RandomEngine& engine,
    size_t cameraId)
{
     auto trace = [&](RandomEngine& engine, Ray ray) -> vec3 {
        return _traceEye(engine, ray);
    };

    for_each_ray(view, engine, _scene->cameras(), cameraId, trace);
}

vec3 Technique::_traceEye(
    RandomEngine& engine,
    const Ray& ray)
{
    return vec3(1.0f, 0.0f, 1.0f);
}

}
