#include <PaintingModelClass.hpp>

TAGPaintingModel::TAGPaintingModel(const std::vector<std::string>& paths, const TAGTexLoader::Params& tex_params, const TAGMesh::Material& material) : TAGModel(tex_params) {
	for (const std::string& path : paths) {
		loadPainting(path, tex_params, material);
	}
}

TAGPaintingModel::TAGPaintingModel(const TAGTexLoader::Params& tex_params, const std::string& path, const TAGMesh::Material& material) : TAGModel(tex_params) {
	if (path != "") {
		loadPainting(path, tex_params, material);
	}
}

void TAGPaintingModel::loadPainting(const std::string& path, const TAGTexLoader::Params& tex_params, const TAGMesh::Material& material) {
	const TAGTexLoader::Info tex_info = TAGTexLoader::loadRawImageData(TAGResourceManager::asset_path + path, tex_params.flip);

	std::vector<TAGMesh::Vertex> vertices;
	vertices.reserve(4);
	const float angle = glm::atan((float)(tex_info.height) / (float)(tex_info.width));
	const std::array<float, 4> angles = { angle, glm::pi<float>() - angle, glm::pi<float>() + angle, glm::two_pi<float>() - angle };
	for (size_t i = 0; i < 4; i++) {
		vertices.push_back({ glm::normalize(glm::mat3(glm::rotate(glm::mat4(1), angles[i], defNormal)) * defArm), defNormal, tex_coords[i] });
	}

	const std::string name = static_cast<std::filesystem::path>(path).stem().string();
	TAGMesh::Material new_material = material;
	TAGTexLoader::Texture texture = TAGTexLoader::textureFromInfo(tex_info, name, tex_params);
	texture.type = TAGTexType::DIFFUSE_MAP;
	new_material.textures.push_back(texture);
	addMesh(name, vertices, { frag_1, frag_2 }, { new_material });
}

void TAGPaintingModel::loadPainting(const std::string& name, const TAGTexLoader::Texture& texture, const TAGMesh::Material& material) {
	std::vector<TAGMesh::Vertex> vertices;
	vertices.reserve(4);
	const float angle = glm::atan((float)(texture.height) / (float)(texture.width));
	const std::array<float, 4> angles = { angle, glm::pi<float>() - angle, glm::pi<float>() + angle, glm::two_pi<float>() - angle };
	for (size_t i = 0; i < 4; i++) {
		vertices.push_back({ glm::normalize(glm::mat3(glm::rotate(glm::mat4(1), angles[i], defNormal)) * defArm), defNormal, tex_coords[i] });
	}

	TAGMesh::Material new_material = material;
	new_material.textures.push_back(texture);

	addMesh(name, vertices, { frag_1, frag_2 }, { new_material });
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
