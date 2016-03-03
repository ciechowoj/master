#pragma once
#include <scene.hpp>

using trace_t = intersect_t;

class octree_t {
public:
	octree_t(const scene_t& scene, size_t depth);



	trace_t trace(const ray_t& ray);

	aabb_t aabb() const;

private:
	struct face_t {
		unsigned shape;
		unsigned face;
	};

	struct node_t {
		node_t* children[2][2][2];
	};

	struct leaf_t : public node_t {
		std::vector<face_t> faces;
	};

	struct cell_t {
		vec3 origin;
		float size;
	};

	cell_t _cell;
	const scene_t& _scene;
	const size_t _depth;
	node_t* _root;

	void _set_origin_and_size(
		const aabb_t& aabb);

	bool _intersects(
		const cell_t& cell,
		const face_t& face) const;

	void _insert_face(
		node_t* node,
		const cell_t& cell,
		const face_t& face,
		size_t depth);

	void _delete_nodes();
	void _delete_subnodes(
		node_t* node);
};
