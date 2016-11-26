#include <Cameras.hpp>
#include <runtime_assert>
#include <unittest>

namespace haste {

size_t Cameras::addCameraFovX(const string& name, const vec3& position,
                              const vec3& direction, const vec3& up, float fovx,
                              float near, float far) {
  _names.push_back(name);

  Desc desc;
  desc.position = position;
  desc.direction = direction;
  desc.up = up;
  desc.fovx = fovx;
  desc.fovy = NAN;
  desc.near = near;
  desc.far = far;

  _descs.push_back(desc);

  float focalInv = tan(fovx * 0.5f);

  _focals.push_back(1.0f / focalInv);

  return _names.size() - 1;
}

const size_t Cameras::numCameras() const { return _names.size(); }

const string& Cameras::name(size_t cameraId) const {
  runtime_assert(cameraId < _names.size());
  return _names[cameraId];
}

const size_t Cameras::cameraId(const string& name) const {
  for (size_t i = 0; i < _names.size(); ++i) {
    if (_names[i] == name) {
      return i;
    }
  }

  return InvalidCameraId;
}

const vec3& Cameras::position(size_t cameraId) const {
  runtime_assert(cameraId < _names.size());
  return _descs[cameraId].position;
}

const vec3& Cameras::direction(size_t cameraId) const {
  runtime_assert(cameraId < _names.size());
  return _descs[cameraId].direction;
}

const vec3& Cameras::up(size_t cameraId) const {
  runtime_assert(cameraId < _names.size());
  return _descs[cameraId].up;
}

const float Cameras::near(size_t cameraId) const {
  runtime_assert(cameraId < _names.size());
  return _descs[cameraId].near;
}

const float Cameras::far(size_t cameraId) const {
  runtime_assert(cameraId < _names.size());
  return _descs[cameraId].far;
}

const float Cameras::fovx(size_t cameraId, float aspect) const {
  runtime_assert(cameraId < _names.size());

  float xfovx = _descs[cameraId].fovx;
  float yfovx = 2.0f * atan(0.5f * _focals[cameraId] * aspect);

  return isnan(_descs[cameraId].fovx) ? yfovx : xfovx;
}

const float Cameras::fovy(size_t cameraId, float aspect) const {
  runtime_assert(cameraId < _names.size());

  float yfovy = _descs[cameraId].fovy;
  float xfovy = 2.0f * atan2(1.0f / aspect, _focals[cameraId]);

  return isnan(_descs[cameraId].fovx) ? yfovy : xfovy;
}

const mat4 Cameras::proj(size_t cameraId, float aspect) const {
  const float fovy = this->fovy(cameraId, aspect);
  return perspective(fovy, aspect, near(cameraId), far(cameraId));
}

const mat4 Cameras::view_to_world_mat4(size_t camera_id) const {
  return inverse(world_to_view_mat4(camera_id));
}

const mat4 Cameras::world_to_view_mat4(size_t camera_id) const {
  auto desc = _descs[camera_id];
  return glm::lookAt(desc.position, desc.position + desc.direction, desc.up);
}

const mat3 Cameras::view_to_world_mat3(size_t camera_id) const {
  return inverse(world_to_view_mat3(camera_id));
}

const mat3 Cameras::world_to_view_mat3(size_t camera_id) const {
  return transpose(inverse(mat3(world_to_view_mat4(camera_id))));
}

const float Cameras::focal_length_y(size_t camera_id, float aspect) const {
  return normalized_flength_y(fovy(camera_id, aspect));
}

float normalized_flength_y(float fov_y) { return 1.0f / tan(fov_y * 0.5f); }

float fov_x(float fov_y, vec2 resolution) { return 0.0f; }

vec3 ray_direction(vec2 position, vec2 resolution, float resolution_y_inv,
                   float normalized_flength_y) {
  float x =
      position.x * resolution_y_inv * 2.0f - resolution.x * resolution_y_inv;
  float y = position.y * resolution_y_inv * 2.0f - 1.0f;

  return normalize(vec3(x, y, -normalized_flength_y));
}

vec3 ray_direction(vec2 position, vec2 resolution, float fov_y) {
  return ray_direction(position, resolution, 1.0f / resolution.y,
                       normalized_flength_y(fov_y));
}

vec2 pixel_position(vec3 direction, vec2 resolution, float resolution_y_inv,
                    float normalized_flength_y) {
  float factor = normalized_flength_y / -direction.z;
  float x = direction.x * factor;
  float y = direction.y * factor;

  y = (y + 1.0f) * resolution.y * 0.5f;
  x = (x + resolution.x * resolution_y_inv) * resolution.y * 0.5f;

  return vec2(x, y);
}

vec2 pixel_position(vec3 direction, vec2 resolution, float fov_y) {
  return pixel_position(direction, resolution, 1.0f / resolution.y,
                        normalized_flength_y(fov_y));
}

mat3 camera_tangent_space(const mat4& world_to_view_mat4) {
  mat3 result;
  result[0] = world_to_view_mat4[1].xyz();
  result[1] = world_to_view_mat4[2].xyz();
  result[2] = world_to_view_mat4[0].xyz();
  return result;
}

vec3 camera_position(const mat4& world_to_view_mat4) {
  vec4 position = inverse(world_to_view_mat4) * vec4(0.0f, 0.0f, 0.0f, 1.0f);
  return position.xyz() / position.w;
}

unittest() {
  vec2 resolution = vec2(800.0f, 600.0f);
  float fov_y = half_pi<float>();

  vec2 expected = vec2(123.4f, 345.0f);
  vec3 direction = ray_direction(expected, resolution, fov_y);
  vec2 actual = pixel_position(direction, resolution, fov_y);

  assert_almost_eq(expected, actual);
}

unittest() {
  vec3 position = vec3(1.f, 3.f, 2.f);
  vec3 direction = vec3(0.0f, 0.0f, -1.0f);
  vec3 up = vec3(0.0f, 1.0f, 0.0f);

  mat4 view = glm::lookAt(position, position + direction, up);

  assert_almost_eq(camera_position(view), position);

  mat3 tangent = camera_tangent_space(view);

  assert_almost_eq(tangent[0], vec3(0.0f, 1.0f, 0.0f));
  assert_almost_eq(tangent[1], vec3(0.0f, 0.0f, 1.0f));
  assert_almost_eq(tangent[2], vec3(1.0f, 0.0f, 0.0f));
}

}
