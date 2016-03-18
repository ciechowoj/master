#include <raycast.hpp>

namespace haste {

vec3 raycast(
	const Ray& ray,
	const Scene& scene)
{
	auto intersect = scene.intersect(ray.origin, ray.direction);

	if (intersect.hit() && scene.isMesh(intersect)) {
		auto light = scene.sampleLight();

		auto& material = scene.material(intersect);

		vec3 normal = scene.lerpNormal(intersect);
		vec3 tangent = vec3(1, 0, 0); // dummy

		vec3 incident = light.position - intersect.position;
		float distanceInv = 1.f / length(incident);
		incident *= distanceInv;
		vec3 reflected = -ray.direction;

		float V = scene.occluded(intersect.position, light.position);
		float G = dot(incident, normal) * dot(reflected, normal) * distanceInv;

		vec3 f = material.brdf(normal, tangent, incident, reflected);

		return f * light.power * V * G;
	}
	else {
		return vec3(0.f);
	}
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
