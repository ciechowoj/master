#pragma once
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <tiny_obj_loader.h>
#include <glm/glm.hpp>

namespace obj = tinyobj;

namespace tinyobj {

struct scene_t {
	std::vector<obj::shape_t> shapes;
	std::vector<obj::material_t> materials;
};

scene_t load(const std::string& obj);

}

using namespace glm; 
using namespace obj;

using scene_t = obj::scene_t;


struct intersect_t {
    size_t shape = 0;
    size_t face = 0;
    float depth = NAN;
};

struct ray_t {
    vec3 pos, dir;

    ray_t()
        : pos(0.0f)
        , dir(0.0f, 0.0f, -1.0f) {
    }

    ray_t(float px, float py, float pz, float dx, float dy, float dz)
        : pos(px, py, pz)
        , dir(dx, dy, dz) { 
    }
};



struct aabb_t {
	vec3 a, b;
};

aabb_t aabb(const scene_t& scene);

bool contains(const aabb_t& aabb, const shape_t& mesh, int face);







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
	std::ostream& ostream,
	const obj::material_t& material);

std::ostream& operator<<(
	std::ostream& stream,
	const aabb_t& aabb);






