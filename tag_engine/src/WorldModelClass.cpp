#include <WorldModelClass.hpp>

TAGWorldModel::TAGWorldModel(const TAGTexLoader::Params& tex_params, const std::string& path) : TAGModel(tex_params, path) {}


TAGMesh::PlaneVolume TAGWorldModel::planeToGameSpace(const TAGMesh::PlaneVolume& plane, const Object& obj) {
	const glm::mat3 rot_mat = glm::mat3(glm::rotate(glm::mat4(1.0f), obj.angle, obj.rotation_axis));
	const TAGMesh::Plane new_frag_plane =
	{
		.normal = rot_mat * plane.frag_plane.normal,
		.start = (rot_mat * plane.frag_plane.start * obj.scale) + obj.position,
		.axis = {rot_mat * plane.frag_plane.axis[0] * obj.scale, rot_mat * plane.frag_plane.axis[1] * obj.scale}
	};
	std::vector<TAGMesh::DotPlane> new_volume_planes;
	new_volume_planes.reserve(3);
	for (size_t i = 0; i < 3; i++) {
		const glm::vec3 normal = rot_mat * plane.volume_planes[i].normal;
		new_volume_planes.emplace_back(normal, glm::dot(normal, (i != 1 ? new_frag_plane.start : new_frag_plane.start + new_frag_plane.axis[0])));
	}
	return { new_frag_plane, *reinterpret_cast<std::array<TAGMesh::DotPlane, 3>*>(new_volume_planes.data()) };
}
