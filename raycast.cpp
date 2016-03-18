#include <raycast.hpp>

namespace haste {

vec3 raycast(
	const Ray& ray,
	const Scene& scene)
{





}

size_t raycastInteractive(
	std::vector<vec4>& image,
	size_t pitch,
	const Camera& camera,
	const Scene& scene)
{
	return renderInteractive(image, pitch, camera, [&](Ray ray) -> vec3 {
        return raycast(ray, scene);
    });
}

}
