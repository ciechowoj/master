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

class CameraBSDF : public BSDF {
public:
    CameraBSDF();
    CameraBSDF(CameraBSDF&&);
    CameraBSDF(const CameraBSDF&);

    CameraBSDF& operator=(CameraBSDF&&);
    CameraBSDF& operator=(const CameraBSDF&);

    const vec3 query(
        const vec3& a,
        const vec3& b) const override;

    const float density(
        const vec3& a,
        const vec3& b) const override;

    const float densityRev(
        const vec3& a,
        const vec3& b) const override;

    const BSDFSample sample(
        RandomEngine& engine,
        const vec3& omega) const override;

    BSDFSample scatter(
        RandomEngine& engine,
        const SurfacePoint& point,
        const vec3& omega) const override;
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
    const BSDF* cameraBSDF(size_t cameraId) const;
    const vec3& position(size_t cameraId) const;
    const vec3& direction(size_t cameraId) const;
    const vec3& up(size_t cameraId) const;
    const float near(size_t cameraId) const;
    const float far(size_t cameraId) const;
    const float fovx(size_t cameraId, float aspect) const;
    const float fovy(size_t cameraId, float aspect) const;
    const mat4 view(size_t cameraId) const;
    const mat4 proj(size_t cameraId, float aspect) const;

    const Ray shoot(
        size_t cameraId,
        const vec2& uniform,
        float widthInv,
        float heightInv,
        float aspect,
        float x,
        float y) const;

    const Ray shoot(
        size_t cameraId,
        RandomEngine& engine,
        float widthInv,
        float heightInv,
        float aspect,
        float x,
        float y) const;

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
    vector<mat4> _views;
    vector<float> _focals;
    CameraBSDF _bsdf;

    const mat4 _view(size_t cameraId) const;
};

}
