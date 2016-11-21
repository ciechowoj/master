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

struct render_context_t {
    mat3 view_to_world_mat3;
    mat3 world_to_view_mat3;
    vec3 camera_position;
    vec2 resolution;
    float resolution_y_inv;
    float focal_length_y;

    RandomEngine* engine;
    vec2 pixel_position;
};

class Technique {
public:
    Technique(size_t num_threads);
    virtual ~Technique();

    virtual void preprocess(
        const shared<const Scene>& scene,
        RandomEngine& engine,
        const function<void(string, float)>& progress,
        bool parallel = false);

    virtual void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId);

    virtual string name() const = 0;

    const size_t numNormalRays() const { return _numNormalRays; }
    const size_t numShadowRays() const { return _numShadowRays; }
    const size_t numSamples() const { return _numSamples; }

protected:
    size_t _numNormalRays;
    size_t _numShadowRays;
    size_t _numSamples;
    shared<const Scene> _scene;
    std::vector<vec3> _helper_image;
    std::mutex _helper_mutex;

    threadpool_t _threadpool;

    virtual vec3 _traceEye(render_context_t& context, Ray ray);

    void _adjust_helper_image(ImageView& view);
    void _trace_paths(ImageView& view, render_context_t& context, size_t cameraId);
    void _commit_helper_image(ImageView& view);

    vec3 _accumulate(render_context_t& context, vec3 radiance, vec3 direction);

    void _for_each_ray(
        ImageView& view,
        render_context_t& context);

private:
    Technique(const Technique&) = delete;
    Technique& operator=(const Technique&) = delete;
};

}
