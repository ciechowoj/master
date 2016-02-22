#pragma once
#include <iostream>
#include <glm/glm.hpp>

std::ostream& operator<<(std::ostream& stream, const vec3& vector) {
	return stream << "[" << vector.x << ", " << vector.y << ", " << vector.z << "]";
}




