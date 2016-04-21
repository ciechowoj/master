#include <runtime_assert>
#include <Cameras.hpp>

namespace haste {

size_t Cameras::addCameraFovX(
    const string& name,
    const vec3& position,
    const vec3& direction,
    const vec3& up,
    float fovx,
    float near,
    float far)
{
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
    _views.push_back(_view(_names.size() - 1));

    float focal = tan(fovx * 0.5f);

    _focalInvs.push_back(1.0f / focal);

    return _names.size() - 1;
}

const size_t Cameras::numCameras() const {
    return _names.size();
}

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
    float yfovx = 2.0f * atan(0.5f * _focalInvs[cameraId] * aspect);

    return isnan(_descs[cameraId].fovx) ? yfovx : xfovx;
}

const float Cameras::fovy(size_t cameraId, float aspect) const {
    runtime_assert(cameraId < _names.size());

    float yfovy = _descs[cameraId].fovy;
    float xfovy = 2.0f * atan(_focalInvs[cameraId] / aspect);

    return isnan(_descs[cameraId].fovx) ? yfovy : xfovy;
}

const mat4 Cameras::view(size_t cameraId) const {
    runtime_assert(cameraId < _names.size());

    return _views[cameraId];
}

const mat4 Cameras::proj(size_t cameraId, float aspect) const {
    const float fovy = this->fovy(cameraId, aspect);
    return perspective(fovy, aspect, near(cameraId), far(cameraId));
}

const Ray Cameras::shoot(
    size_t cameraId,
    const vec2& uniform,
    float widthInv,
    float heightInv,
    float aspect,
    float x,
    float y) const
{
    runtime_assert(cameraId < _names.size());

    float sx = (x + uniform.x) * widthInv * 2.0f - 1.f;
    float sy = (y + uniform.y) * heightInv * 2.0f - 1.f;

    float focalInv = _focalInvs[cameraId];
    const mat4& view = this->view(cameraId);

    vec3 direction = normalize(vec3(sx * aspect, sy, -focalInv));

    return { view[3].xyz(), view * vec4(direction, 0.0f) };
}

const Ray Cameras::shoot(
    size_t cameraId,
    RandomEngine& engine,
    float widthInv,
    float heightInv,
    float aspect,
    float x,
    float y) const
{
    runtime_assert(cameraId < _names.size());

    auto uniform = sampleUniform2(engine);
    return shoot(cameraId, uniform.value(), widthInv, heightInv, aspect, x, y);
}

const mat4 Cameras::_view(size_t cameraId) const {
    vec3 y = up(cameraId);
    vec3 z = -direction(cameraId);
    vec3 x = cross(y, z);
    y = cross(z, x);

    x = normalize(x);
    y = normalize(y);
    z = normalize(z);

    mat4 result;
    result[0] = vec4(x, 0.0f);
    result[1] = vec4(y, 0.0f);
    result[2] = vec4(z, 0.0f);
    result[3] = vec4(position(cameraId), 1.0f);

    return result;
}

}
