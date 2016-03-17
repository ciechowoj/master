#pragma once
#include <scene.hpp>
#include <camera.hpp>

namespace haste {

void raycast(
	std::vector<vec3>& imageData,
	const ImageDesc& imageDesc,
	const Camera& camera,
	const Scene& scene,
	size_t numSamples);

}
