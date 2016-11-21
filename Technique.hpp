#pragma once
#include <cmath>
#include <ctgmath>
#include <runtime_assert>
#include <Prerequisites.hpp>
#include <ImageView.hpp>
#include <Scene.hpp>
#include <threadpool.hpp>
#include <mutex>

namespace haste {

struct camera_t {
    mat4 view;
    mat4 view_inv;
    vec2 resolution;
    float resolution_y_inv;
    float normalized_focal_length_y;


};

inline vec3 direction_to_view_space(const camera_t& camera, vec3 direction) { }

struct render_context_t {
    RandomEngine* engine;
    camera_t camera;
    vec2 pixel_position;
};


class Technique {
public:
    Technique();
    virtual ~Technique();

    virtual void preprocess(
        const shared<const Scene>& scene,
        RandomEngine& engine,
        const function<void(string, float)>& progress,
        bool parallel = false);

    virtual void render(
        ImageView& view,
        render_context_t& context,
        size_t cameraId,
        bool parallel);

    virtual void render(
        ImageView& view,
        render_context_t& context,
        size_t cameraId);

    virtual string name() const = 0;

    const size_t numNormalRays() const { return _numNormalRays; }
    const size_t numShadowRays() const { return _numShadowRays; }
    const size_t numSamples() const { return _numSamples; }

    template <class F> static void for_each_ray(
        ImageView& view,
        render_context_t& context,
        const Cameras& cameras,
        size_t cameraId,
        const F& func);
protected:
    size_t _numNormalRays;
    size_t _numShadowRays;
    size_t _numSamples;
    shared<const Scene> _scene;
    std::vector<vec3> _helper_image;
    std::mutex _helper_mutex;

    threadpool_t _threadpool;

    virtual vec3 _traceEye(
        render_context_t& context,
        const Ray& ray);

    void _adjust_helper_image(ImageView& view);
    void _trace_paths(ImageView& view, render_context_t& context, size_t cameraId, bool parallel);
    void _commit_helper_image(ImageView& view, bool parallel);
    void _commit_helper_image(ImageView& view);

    vec3 _accumulate(render_context_t& context, vec3 radiance, vec3 direction);

private:
    Technique(const Technique&) = delete;
    Technique& operator=(const Technique&) = delete;
};

template <class F> inline void Technique::for_each_ray(
    ImageView& view,
    render_context_t& context,
    const Cameras& cameras,
    size_t cameraId,
    const F& func)
{
    const int xBegin = int(view.xBegin());
    const int xEnd = int(view.xEnd());
    const int rXBegin = int(xEnd - 1);
    const int rXEnd = int(xBegin - 1);
    const int yBegin = int(view.yBegin());
    const int yEnd = int(view.yEnd());

    runtime_assert(0 <= xBegin && xEnd <= view.width());
    runtime_assert(0 <= yBegin && yEnd <= view.height());

    const float width = float(view.width());
    const float height = float(view.height());
    const float widthInv = 1.0f / width;
    const float heightInv = 1.0f / height;
    const float aspect = width / height;

    auto shoot = [&](float x, float y) -> Ray {
        return cameras.shoot(
            cameraId,
            *context.engine,
            widthInv,
            heightInv,
            aspect,
            float(x),
            float(y));
    };

    for (int y = yBegin; y < yEnd; ++y) {
        for (int x = xBegin; x < xEnd; ++x) {
            const Ray ray = shoot(float(x), float(y));
            vec3 radiance = func(context, ray);
            float cumulative = radiance.x + radiance.y + radiance.z;
            view.absAt(x, y) += std::isfinite(cumulative) ? vec4(radiance, 1.0f) : vec4(0.0f);
        }

        ++y;

        if (y < yEnd) {
            for (int x = rXBegin; x > rXEnd; --x) {
                const Ray ray = shoot(float(x), float(y));
                vec3 radiance = func(context, ray);
                float cumulative = radiance.x + radiance.y + radiance.z;
                view.absAt(x, y) += std::isfinite(cumulative) ? vec4(radiance, 1.0f) : vec4(0.0f);
            }
        }
    }
}

}
