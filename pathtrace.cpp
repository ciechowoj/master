#include <pathtrace.hpp>

namespace haste {

vec3 sampleLight(
    const Scene& scene,
    const vec3& position,
    const vec3& normal,
    const vec3& reflected,
    const mat3& worldToLight,
    const BSDF& bsdf);

vec3 pathtrace(
    Ray ray,
    const Scene& scene);

size_t pathtraceInteractive(
    std::vector<vec4>& image,
    size_t pitch,
    const Camera& camera,
    const Scene& scene)
{
	return renderInteractive(image, pitch, camera, [&](Ray ray) -> vec3 {
        return pathtrace(ray, scene);
    });
}

}
