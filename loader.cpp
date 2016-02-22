#include <loader.hpp>

scene_t load_obj(const std::string& obj) {
	std::string errors;

	scene_t scene;

	bool result = tinyobj::LoadObj(
		scene.shapes,
		scene.materials,
		errors,
		obj.c_str());

	if (!errors.empty()) {
		std::cerr << errors << std::endl;
	}

	if (!result) {
		throw std::runtime_error(errors);
	}

	return scene;
}

std::ostream& operator<<(
	std::ostream& stream, 
	const tinyobj::shape_t& shape) {
	stream << "shape_t(name = " << shape.name << ", mesh = mesh_t(...))";
	return stream;
}

std::ostream& operator<<(
	std::ostream& stream, 
	const std::vector<tinyobj::shape_t>& shapes) {
	stream << "[";

	if (shapes.size() != 0) {
		for (size_t i = 0; i < shapes.size() - 1; ++i) {
			stream << shapes[i] << ", ";
		}
		
		stream << shapes[shapes.size() - 1];
	}

	stream << "]";

	return stream;
}

std::ostream& operator<<(
	std::ostream& stream,
	const std::vector<float>& floats) {

	stream << "[";

	if (floats.size() != 0) {
		for (size_t i = 0; i < floats.size() - 1; ++i) {
			stream << floats[i] << ", ";
		}
		
		stream << floats[floats.size() - 1];
	}

	stream << "]";

	return stream;
}
