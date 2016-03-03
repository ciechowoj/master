#include <octree.hpp>


octree_t::octree_t(const scene_t& scene, size_t depth)
	: _scene(scene) 
	, _depth(depth)
{
	_set_origin_and_size(::aabb(scene));

	for (size_t s = 0; s < scene.shapes.size(); ++s) {
		const auto& mesh = scene.shapes[s].mesh;

		for (size_t i = 0; i < mesh.indices.size(); i += 3) {

		}
	}
 

}

aabb_t octree_t::aabb() const {
	aabb_t result;
	result.a = _cell.origin;
	result.b = _cell.origin + _cell.size;
	return result;
}

void octree_t::_set_origin_and_size(
	const aabb_t& aabb) 
{
	vec3 size = aabb.b - aabb.a;
	_cell.size = max(size.x, max(size.y, size.z));
	_cell.origin = aabb.a - (_cell.size - size) * 0.5f;
}

bool octree_t::_intersects(
	const cell_t& cell,
	const face_t& face) const {
	
}

void octree_t::_insert_face(
	node_t* node,
	const cell_t& cell,
	const face_t& face,
	size_t depth)
{
	for (size_t k = 0; k < 2; ++k) {
		for (size_t j = 0; j < 2; ++j) {
			for (size_t i = 0; i < 2; ++i) {
				cell_t subcell;
				subcell.size = cell.size * 0.5f;
				subcell.origin = cell.origin + subcell.size * vec3(i, j, k);

				if (_intersects(subcell, face)) {
					if (depth == _depth) {
						if (!node->children[k][j][i]) {
							node->children[k][j][i] = new leaf_t();
						}

						auto leaf = (leaf_t*)node->children[k][j][i];
						leaf->faces.push_back(face);
					}
					else {
						if (!node->children[k][j][i]) {
							node->children[k][j][i] = new node_t();
						}

						_insert_face(node->children[k][j][i], subcell, face, depth + 1);
					}
				}
			}
		}
	}
}

void octree_t::_delete_nodes() 
{
	if (_root != nullptr) {
		_delete_subnodes(_root);
		delete _root;
		_root = nullptr;
	}
}

void octree_t::_delete_subnodes(
	node_t* node)
{
	for (size_t k = 0; k < 2; ++k) {
		for (size_t j = 0; j < 2; ++j) {
			for (size_t i = 0; i < 2; ++i) {
				if (node->children[k][j][i] != nullptr) {
					_delete_subnodes(node->children[k][j][i]);
					delete node->children[k][j][i];
					node->children[k][j][i] = nullptr;
				}
			}
		}
	}
}
