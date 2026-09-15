#include <WorldModelClass.hpp>

TAGWorldModel::TAGWorldModel(const TAGTexLoader::Params& tex_params, const TAGResourceManager::BufferAccess& access, const std::string& path) : TAGModel(tex_params, access, path) {}

Collision::Info TAGWorldModel::collisionToGameSpace(const Collision::Info& collision, const Object& obj) {
	const glm::mat3 rot_mat = glm::mat3(glm::rotate(glm::mat4(1.0f), obj.angle, obj.rotation_axis));
	const TAGMesh::Plane new_frag_plane =
	{
		.normal = rot_mat * collision.plane.frag_plane.normal,
		.start = (rot_mat * collision.plane.frag_plane.start * obj.scale) + obj.position,
		.axis = { rot_mat * collision.plane.frag_plane.axis[0] * obj.scale, rot_mat * collision.plane.frag_plane.axis[1] * obj.scale }
	};
	std::array<TAGMesh::DotPlane, 3> new_volume_planes = {};
	for (size_t i = 0; i < 3; i++) {
		const glm::vec3 normal = rot_mat * collision.plane.volume_planes[i].normal;
		new_volume_planes[0] = { normal, glm::dot(normal, (i != 1 ? new_frag_plane.start : new_frag_plane.start + new_frag_plane.axis[0])) };
	}

	const glm::vec3 new_normal = rot_mat * collision.collision_plane.normal;
	const float new_constant = glm::dot(new_normal, (rot_mat * collision.collision_plane.normal * collision.collision_plane.constant * obj.scale) + obj.position);
	return { { new_frag_plane, new_volume_planes }, { new_normal, new_constant } };
}
