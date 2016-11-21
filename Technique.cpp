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
    render_context_t& context,
    size_t cameraId,
    bool parallel)
{
    size_t numNormalRays = _scene->numNormalRays();
    size_t numShadowRays = _scene->numShadowRays();

    _adjust_helper_image(view);
    _trace_paths(view, context, cameraId, parallel);
    _commit_helper_image(view, parallel);

    _numNormalRays += _scene->numNormalRays() - numNormalRays;
    _numShadowRays += _scene->numShadowRays() - numShadowRays;
    _numSamples = size_t(view.last().w);
}

void Technique::render(
    ImageView& view,
    render_context_t& context,
    size_t cameraId)
{
     auto trace = [&](render_context_t& context, Ray ray) -> vec3 {
        return _traceEye(context, ray);
    };

    for_each_ray(view, context, _scene->cameras(), cameraId, trace);
}

vec3 Technique::_traceEye(
    render_context_t& context,
    const Ray& ray)
{
    return vec3(1.0f, 0.0f, 1.0f);
}

void Technique::_adjust_helper_image(ImageView& view) {
    size_t view_size = view.width() * view.height();

    if (_helper_image.size() != view_size) {
        _helper_image.resize(view_size, vec3(0.0f));
    }
}

void Technique::_trace_paths(
    ImageView& view,
    render_context_t& context,
    size_t cameraId,
    bool parallel) {
    if (parallel) {
        exec2d(_threadpool, view.xWindow(), view.yWindow(), 32,
            [&](size_t x0, size_t x1, size_t y0, size_t y1) {
            render_context_t local_context = context;
            RandomEngine engine;
            local_context.engine = &engine;

            ImageView subview = view;

            size_t xBegin = view._xOffset + x0;
            size_t xEnd = view._xOffset + x1;

            size_t yBegin = view._yOffset + y0;
            size_t yEnd = view._yOffset + y1;

            subview._xOffset = xBegin;
            subview._xWindow = xEnd - xBegin;

            subview._yOffset = yBegin;
            subview._yWindow = yEnd - yBegin;

            render(subview, local_context, cameraId);
        });
    }
    else {
        render(view, context, cameraId);
    }
}

void Technique::_commit_helper_image(ImageView& view, bool parallel) {
    if (parallel) {
        exec2d(_threadpool, view.xWindow(), view.yWindow(), 32,
            [&](size_t x0, size_t x1, size_t y0, size_t y1) {
            ImageView subview = view;

            size_t xBegin = view._xOffset + x0;
            size_t xEnd = view._xOffset + x1;

            size_t yBegin = view._yOffset + y0;
            size_t yEnd = view._yOffset + y1;

            subview._xOffset = xBegin;
            subview._xWindow = xEnd - xBegin;

            subview._yOffset = yBegin;
            subview._yWindow = yEnd - yBegin;

            _commit_helper_image(subview);
        });
    }
    else {
        _commit_helper_image(view);
    }
}

void Technique::_commit_helper_image(ImageView& view) {
    for (size_t y = view.yBegin(); y < view.yEnd(); ++y) {
        vec4* dst_begin = view.data() + y * view.width() + view.xBegin();
        vec4* dst_end = dst_begin + view.xWindow();
        vec3* src_itr = _helper_image.data() + y * view.width() + view.xBegin();

        for (vec4* dst_itr = dst_begin; dst_itr < dst_end; ++dst_itr) {
            *dst_itr += vec4(*src_itr, 0.0f);
            *src_itr = vec3(0.0f);
            ++src_itr;
        }
    }
}

vec3 Technique::_accumulate(
    render_context_t& context,
    vec3 radiance,
    vec3 direction) {
    vec3 view_direction = direction_to_view_space(context.camera, direction);

    vec2 position = pixel_position(
        view_direction,
        context.camera.resolution,
        context.camera.resolution_y_inv,
        context.camera.normalized_focal_length_y);

    vec2 position_floor = floor(position);

    if (position == context.pixel_position) {
        return radiance;
    }
    else if (0 <= position_floor.x &&
        position_floor.x < context.camera.resolution.x &&
        0 <= position_floor.y &&
        position_floor.y < context.camera.resolution.y) {
        ivec2 iposition = ivec2(position_floor);
        int width = int(context.camera.resolution.x);

        std::unique_lock<std::mutex> lock(_helper_mutex);
        _helper_image[iposition.y * width + iposition.x] += radiance;
        return vec3(0.0f);
    }
}


}
