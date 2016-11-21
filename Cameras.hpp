#pragma once
#include <Prerequisites.hpp>
#include <BSDF.hpp>
#include <glm>

namespace haste {

static const size_t InvalidCameraId = SIZE_MAX;

struct Ray {
    vec3 origin;
    vec3 direction;
};

class Cameras {
public:
    size_t addCameraFovX(
        const string& name,
        const vec3& position,
        const vec3& direction,
        const vec3& up,
        float fovx,
        float near = 0.1f,
        float far = 100.0f);

    const size_t numCameras() const;
    const string& name(size_t cameraId) const;
    const size_t cameraId(const string& name) const;
    const vec3& position(size_t cameraId) const;
    const vec3& direction(size_t cameraId) const;
    const vec3& up(size_t cameraId) const;
    const float near(size_t cameraId) const;
    const float far(size_t cameraId) const;
    const float fovx(size_t cameraId, float aspect) const;
    const float fovy(size_t cameraId, float aspect) const;
    const mat4 proj(size_t cameraId, float aspect) const;

    const mat4 view_to_world_mat4(size_t camera_id) const;
    const mat4 world_to_view_mat4(size_t camera_id) const;
    const mat3 view_to_world_mat3(size_t camera_id) const;
    const mat3 world_to_view_mat3(size_t camera_id) const;
    const float focal_length_y(size_t camera_id, float aspect) const;
private:
    struct Desc {
        vec3 position;
        vec3 direction;
        vec3 up;
        float fovx;
        float fovy;
        float near;
        float far;
    };

    vector<string> _names;
    vector<Desc> _descs;
    vector<float> _focals;

    const mat4 _view(size_t cameraId) const;
};

float normalized_flength_y(float fov_y);
float fov_x(float fov_y, vec2 resolution);

vec3 ray_direction(vec2 position, vec2 resolution, float resolution_y_inv,
                   float normalized_flength_y);

vec3 ray_direction(vec2 position, vec2 resolution, float fov_y);

vec2 pixel_position(vec3 direction, vec2 resolution, float resolution_y_inv,
                    float normalized_flength_y);

vec2 pixel_position(vec3 direction, vec2 resolution, float fov_y);

}
