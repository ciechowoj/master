#pragma once
#include <iostream>
#include <vector>
#include <glm>

namespace haste {

inline std::ostream& operator<<(std::ostream& stream, const vec3& vector) {
	return stream << "[" << vector.x << ", " << vector.y << ", " << vector.z << "]";
}

template <class T> inline std::ostream& operator<<(
	std::ostream& stream,
	const std::vector<T>& items) {

	stream << "[";

	if (items.size() != 0) {
		for (size_t i = 0; i < items.size() - 1; ++i) {
			stream << items[i] << ", ";
		}
		
		stream << items[items.size() - 1];
	}

	stream << "]";

	return stream;
}

}
