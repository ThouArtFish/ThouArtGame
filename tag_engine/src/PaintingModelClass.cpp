#include <PaintingModelClass.hpp>

TAGPaintingModel::TAGPaintingModel(const std::vector<std::string>& paths, const TAGTexLoader::Params& tex_params, const TAGMesh::MaterialMod& mat_mod) : TAGModel(tex_params) {
	for (size_t i = 0; i < paths.size(); i++) {
		loadPainting(paths[i], tex_params, mat_mod);
	}
}

TAGPaintingModel::TAGPaintingModel(const TAGTexLoader::Params& tex_params, const TAGMesh::MaterialMod& mat_mod, const std::string& path) : TAGModel(tex_params) {
	if (path != "") {
		loadPainting(path, tex_params, mat_mod);
	}
}

void TAGPaintingModel::loadPainting(const std::string& path, const TAGTexLoader::Params& tex_params, const TAGMesh::MaterialMod& mat_mod) {
	const TAGTexLoader::Info tex_info = TAGTexLoader::loadRawImageData(TAGResourceManager::asset_path + path, tex_params.flip);

	std::vector<glm::vec3> corners;
	corners.reserve(4);
	const float angle = glm::atan((float)(tex_info.height) / (float)(tex_info.width));
	const std::array<float, 4> angles = { angle, glm::pi<float>() - angle, glm::pi<float>() + angle, glm::two_pi<float>() - angle };
	for (const float& angle : angles) {
		corners.push_back(glm::normalize(glm::mat3(glm::rotate(glm::mat4(1), angle, defNormal)) * defArm));
	}

	std::vector<TAGMesh::Vertex> vertices;
	vertices.reserve(4);
	for (size_t i = 0; i < 4; i++) {
		vertices.push_back({ corners[i], defNormal, tex_coords[i] });
	}
	
	const std::string name = static_cast<std::filesystem::path>(path).stem().string();
	const TAGMesh::Texture texture = { TAGTexLoader::textureFromInfo(tex_info, tex_params), TAGTexType::DIFFUSE_MAP };
	addMesh(name, vertices, { frag_1, frag_2 }, { texture }, mat_mod);
}

void TAGPaintingModel::faceDirec(const glm::vec3& point, Object& obj, const bool& lock_axis) {
	const glm::vec3 new_normal = glm::normalize(point - obj.position);
	const glm::vec3 axis = glm::cross(new_normal, defNormal);
	float angle;
	if (lock_axis) {
		angle = glm::acos(glm::dot(glm::normalize(new_normal - obj.rotation_axis * glm::dot(obj.rotation_axis, new_normal)), defNormal));
	}
	else {
		obj.rotation_axis = glm::normalize(axis);
		angle = glm::acos(glm::dot(new_normal, defNormal));
	}
	obj.angle = (glm::dot(axis, obj.rotation_axis) > 0.0f ? -angle : angle);
}
