#pragma once
#include <cmath>
#include <ctgmath>
#include <runtime_assert>
#include <Prerequisites.hpp>
#include <ImageView.hpp>
#include <Scene.hpp>
#include <threadpool.hpp>
#include <mutex>
#include <statistics.hpp>

namespace haste {

struct render_context_t {
    mat3 view_to_world_mat3;
    mat3 world_to_view_mat3;
    vec3 camera_position;
    vec2 resolution;
    float resolution_y_inv;
    float focal_length_y;
    float focal_factor_y;
    int32_t camera_id;

    RandomEngine* generator;
    vec2 pixel_position;
    std::size_t pixel_index;
};

class Technique {
public:
    Technique(const shared<const Scene>& scene, size_t num_threads);
    virtual ~Technique();

    virtual void render(
        ImageView& view,
        RandomEngine& engine,
        size_t cameraId,
        const vector<vec3>& reference,
        const vector<ivec2>& trace_points);

    const statistics_t& statistics() const;
    void set_statistics(const statistics_t& statistics);
protected:
    double _start_time = NAN;
    statistics_t _statistics;
    shared<const Scene> _scene;
    std::vector<dvec3> _eye_image;
    std::vector<dvec3> _light_image;
    std::mutex _light_mutex;

    threadpool_t _threadpool;

    virtual vec3 _traceEye(render_context_t& context, Ray ray);
    virtual void _preprocess(RandomEngine& engine, double num_samples);
    static SurfacePoint _camera_surface(render_context_t& context);
    static vec3 _camera_direction(render_context_t& context);

    void _adjust_helper_image(ImageView& view);
    void _trace_paths(ImageView& view, render_context_t& context, size_t cameraId);
    size_t _commit_images(ImageView& view);

    float _normal_coefficient(
        const vec3& light_omega,
        const vec3& light_gnormal,
        const vec3& light_normal,
        const vec3& eye_omega) const;

    float _focal_coefficient(
        const vec3& eye_omega,
        const vec3& eye_normal) const;

    float _camera_coefficient(
        const vec3& light_omega,
        const vec3& light_gnormal,
        const vec3& light_normal,
        const vec3& eye_omega,
        const vec3& eye_normal) const;

    template <class F>
    vec3 _accumulate(
        render_context_t& context,
        vec3 direction,
        F&& callback) {
        return _accumulate(
            context,
            direction,
            &callback,
            [](void* closure) -> vec3 { return (*(F*)closure)(); });
    }

    vec3 _accumulate(
        render_context_t& context,
        vec3 direction,
        void* closure,
        vec3 (*)(void*));

    void _for_each_ray(
        ImageView& view,
        render_context_t& context);

    void _make_measurements(
      const vector<ivec2>& trace_points,
      image_view_t<dvec4> a,
      image_view_t<vec3> b);

private:
    Technique(const Technique&) = delete;
    Technique& operator=(const Technique&) = delete;
};

}
