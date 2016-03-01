#pragma once
#include <iostream>
#include <stdexcept>
#include <tiny_obj_loader.h>

namespace obj = tinyobj;

namespace tinyobj {

struct scene_t {
	std::vector<obj::shape_t> shapes;
	std::vector<obj::material_t> materials;
};

scene_t load(const std::string& obj);

}

std::ostream& operator<<(
	std::ostream& stream,
	const obj::scene_t& scene);

std::ostream& operator<<(
	std::ostream& stream, 
	const obj::shape_t& shape);

std::ostream& operator<<(
	std::ostream& ostream,
	const obj::mesh_t& shape);

std::ostream& operator<<(
	std::ostream& stream, 
	const std::vector<obj::shape_t>& shapes);

std::ostream& operator<<(
	std::ostream& stream,
	const std::vector<float>& floats);


