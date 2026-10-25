#include <ModelClass.hpp>

TAGModel::TAGModel(const TAGTexLoader::Params& tex_params, const TAGResourceManager::BufferAccess& access, const std::string& path) {
	this->tex_params = tex_params;
	this->access = access;
	directory = TAGResourceManager::asset_path + path.substr(0, path.find_last_of("/") + 1);

	if (path != "") loadModel(TAGResourceManager::asset_path + path);
}

void TAGModel::drawAll(const TAGShaderManager::Shader& shader, const std::string& mesh_name, const TAGShaderManager::ShaderOptions& options) {
	if ((mesh_name != "" && meshes.find(mesh_name) == meshes.end()) || instance_buffers.find(mesh_name) == instance_buffers.end()) return;

	InstanceBuffer& instance_buffer = instance_buffers.at(mesh_name);

	for (const auto& pair : shader.buffer_locations) {
		TAGResourceManager::updateAttachedBuffers((TAGResourceManager::ShaderBufferType)pair.first, pair.second);
	}

	if (!options.cull_backface) glDisable(GL_CULL_FACE);

	if (instance_buffer.isObjectsChanged()) updateInstanceBuffer(mesh_name);

	const auto& buffer = instance_buffer.getBuffer();

	if (mesh_name != "") {
		TAGMesh& mesh = meshes.at(mesh_name);

		glVertexArrayVertexBuffer(mesh.VAO, TAGMesh::default_vao_instance_binding_point, buffer->getBufferID(), buffer->getInternalOffset(), sizeof(ShaderObject));

		mesh.draw(shader, options, buffer->getCurrentObjects());

		glVertexArrayVertexBuffer(mesh.VAO, TAGMesh::default_vao_instance_binding_point, 0, 0, 0);
	}
	else {
		for (auto& pair : meshes) {
			glVertexArrayVertexBuffer(pair.second.VAO, TAGMesh::default_vao_instance_binding_point, buffer->getBufferID(), buffer->getInternalOffset(), sizeof(ShaderObject));

			pair.second.draw(shader, options, buffer->getCurrentObjects());

			glVertexArrayVertexBuffer(pair.second.VAO, TAGMesh::default_vao_instance_binding_point, 0, 0, 0);
		}
	}

	instance_buffer.setFence();

	for (const auto& pair : shader.buffer_locations) {
		TAGResourceManager::fenceAttachedBuffers((TAGResourceManager::ShaderBufferType)pair.first, pair.second);
	}

	if (!options.cull_backface) glEnable(GL_CULL_FACE);
}

void TAGModel::drawOne(const TAGShaderManager::Shader& shader, const Object& obj, const std::string& mesh_name, const TAGShaderManager::ShaderOptions& options) {
	if (mesh_name != "" && meshes.find(mesh_name) == meshes.end()) return;

	for (const auto& pair : shader.buffer_locations) {
		TAGResourceManager::updateAttachedBuffers((TAGResourceManager::ShaderBufferType)pair.first, pair.second);
	}

	if (!options.cull_backface) glDisable(GL_CULL_FACE);
	
	const std::array<glm::vec4, 2> shader_object = { glm::vec4(obj.position, obj.scale), glm::vec4(obj.rotation_axis, obj.angle) };
	shader.set<glm::vec4>(options.shader_object, shader_object[0], 2);
	if (mesh_name != "") {
		meshes.at(mesh_name).draw(shader, options);
	}
	else {
		for (auto& pair : meshes) {
			pair.second.draw(shader, options);
		}
	}

	for (const auto& pair : shader.buffer_locations) {
		TAGResourceManager::fenceAttachedBuffers((TAGResourceManager::ShaderBufferType)pair.first, pair.second);
	}

	if (!options.cull_backface) glEnable(GL_CULL_FACE);
}

void TAGModel::setInstance(const Object& obj, const int& index, const std::string& mesh_name) {
	if (mesh_name != "" && meshes.find(mesh_name) == meshes.end()) return;

	auto& obj_buffer = instance_buffers.try_emplace(mesh_name, default_instance_buffer_size, shaderConverter, access).first->second;
	if (index < 0) {
		obj_buffer.pushObject(obj);
	}
	else if (index < obj_buffer.getAllObjects().size()) {
		obj_buffer.setObject(obj, index);
	}
}

TAGModel::Object TAGModel::removeInstance(const int& index, const std::string& mesh_name) {
	auto& obj_buffer = instance_buffers.at(mesh_name);

	return (index < 0 ? obj_buffer.popObject() : obj_buffer.removeObject(index));
}

void TAGModel::setAllInstances(const std::vector<Object>& objs, const std::string& mesh_name) {
	if (mesh_name != "" && meshes.find(mesh_name) == meshes.end()) return;

	auto& obj_buffer = instance_buffers.try_emplace(mesh_name, default_instance_buffer_size, shaderConverter, access).first->second;
	obj_buffer.setAllObjects(objs);
}

const TAGModel::Object& TAGModel::getInstance(const int& index, const std::string& mesh_name) const {
	auto& obj_buffer = instance_buffers.at(mesh_name);

	return (index < 0 ? obj_buffer.peekObject() : obj_buffer.getObject(index));
}

const std::vector<TAGModel::Object>& TAGModel::getAllInstances(const std::string& mesh_name) const {
	return instance_buffers.at(mesh_name).getAllObjects();
}

void TAGModel::updateInstanceBuffer(const std::string& mesh_name) {
	if (instance_buffers.contains(mesh_name)) instance_buffers.at(mesh_name).updateBuffer();
}

TAGMesh& TAGModel::getMesh(const std::string& mesh_name) {
	return meshes.at(mesh_name);
}

void TAGModel::addMesh(const std::string& mesh_name, const std::vector<TAGMesh::Vertex>& vertices, const std::vector<TAGMesh::Fragment>& frags, const TAGMesh::Material& material) {
	if (mesh_name == "") return;

	meshes.try_emplace(mesh_name, vertices, frags, material);
}

void TAGModel::deleteMesh(const std::string& mesh_name) {
	if (mesh_name == "") return;

	meshes.erase(mesh_name);
	instance_buffers.erase(mesh_name);
}

void TAGModel::loadModel(const std::string& path) {
	// Parse model file
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;
	const bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), this->directory.c_str());

	if (!warn.empty()) {
		std::cout << "TAGModel : Tinyobjloader Warning :  " << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << "TAGModel : Tinyobjloader Error : " << err << std::endl;
	}

	if (!ret) {
		std::cout << "TAGModel : Failed to parse model" << std::endl;
		return;
	}

	// Get position, normal and tex coords in vector format
	const std::array<size_t, 3> array_sizes = { attrib.vertices.size() / 3, attrib.normals.size() / 3, attrib.texcoords.size() / 2 };
	const size_t max_index = glm::max(glm::max(array_sizes[0], array_sizes[1]), array_sizes[2]);
	std::vector<glm::vec3> all_vertices;
	all_vertices.assign(array_sizes[0], glm::vec3(0));
	std::vector<glm::vec3> all_normals;
	all_normals.assign(array_sizes[1], glm::normalize(glm::vec3(1)));
	std::vector<glm::vec2> all_texcoords;
	all_texcoords.assign(array_sizes[2], glm::vec2(0));
	for (size_t i = 0; i < max_index; i++) {
		if (i < array_sizes[0]) {
			for (size_t j = 0; j < 3; j++) {
				all_vertices[i][j] = attrib.vertices[i * 3 + j];
			}
		}
		if (i < array_sizes[1]) {
			for (size_t j = 0; j < 3; j++) {
				all_normals[i][j] = attrib.normals[i * 3 + j];
			}
			all_normals[i] = glm::normalize(all_normals[i]);
		}
		if (i < array_sizes[2]) {
			for (size_t j = 0; j < 2; j++) {
				all_texcoords[i][j] = attrib.texcoords[i * 2 + j];
			}
		}
	}

	// Load each mesh
	for (const tinyobj::shape_t& shape : shapes) {
		if (shape.name == "") continue;

		// Mesh data
		std::vector<TAGMesh::Vertex> mesh_vertices; mesh_vertices.reserve(shape.mesh.indices.size());
		std::vector<TAGMesh::MaterialFragments> mesh_material_frags; mesh_material_frags.reserve(materials.size());
		std::vector<TAGMesh::Material> mesh_materials; mesh_materials.reserve(materials.size());

		// Indices of materials found in mesh
		std::vector<GLuint> found_materials;

		// Vertices which are part of mesh already
		std::unordered_map<size_t, GLuint> unique_indices;

		// Current primitive
		std::array<GLuint, 3> primitive;

		// Next free index in primitive
		size_t primitive_index = 0;

		// Loop through each vertex in mesh
		for (size_t i = 0; i < shape.mesh.indices.size(); i++) {
			const tinyobj::index_t current_index = shape.mesh.indices[i];
			const size_t index_address = current_index.texcoord_index + (current_index.normal_index + current_index.vertex_index * max_index) * max_index;

			if (!unique_indices.contains(index_address)) {
				unique_indices[index_address] = (GLuint)mesh_vertices.size();
				mesh_vertices.emplace_back(all_vertices[current_index.vertex_index], all_normals[current_index.normal_index], all_texcoords[current_index.texcoord_index]);
			}
			primitive[primitive_index] = unique_indices[index_address];

			if (primitive_index == 2) {
				GLuint material_index = shape.mesh.material_ids[(i - 2) / 3];
				const auto& material_pos = std::find(found_materials.begin(), found_materials.end(), material_index);
				if (material_pos == found_materials.end()) {
					found_materials.push_back(material_index);
					material_index = (GLuint)(found_materials.size() - 1);
				}
				else {
					material_index = (GLuint)std::distance(found_materials.begin(), material_pos);
				}
				
				auto material_frag_pos = std::find_if(mesh_material_frags.begin(), mesh_material_frags.end(), [&material_index](const TAGMesh::MaterialFragments& mat_frags) { return mat_frags.material_index == material_index; });
				if (material_frag_pos == mesh_material_frags.end()) {
					mesh_material_frags.emplace_back(material_index, std::vector<TAGMesh::Fragment>(1, primitive));
				}
				else {
					material_frag_pos->frags.push_back(primitive);
				}
				primitive_index = 0;
			}
			else {
				primitive_index++;
			}
		}

		// Loaded textures
		std::vector<TAGTexLoader::Texture> loaded_textures;

		// Load materials for model
		for (const GLuint& material_index : found_materials) {
			const tinyobj::material_t& material = materials[material_index];
			TAGMesh::Material mesh_material;

			if (!material.diffuse_texname.empty()) {
				const auto& texture_pos = std::find_if(loaded_textures.begin(), loaded_textures.end(), [&material](const TAGTexLoader::Texture& tex) { return tex.name == material.diffuse_texname; });
				if (texture_pos == loaded_textures.end()) {
					mesh_material.textures.push_back(loadMaterialTexture(material.diffuse_texname, TAGTexType::DIFFUSE_MAP));
					loaded_textures.push_back(mesh_material.textures.back());
				}
				else {
					TAGTexLoader::Texture texture = *texture_pos;
					texture.type = TAGTexType::DIFFUSE_MAP;
					mesh_material.textures.push_back(texture);
				}
			}

			if (!material.specular_texname.empty()) {
				const auto& texture_pos = std::find_if(loaded_textures.begin(), loaded_textures.end(), [&material](const TAGTexLoader::Texture& tex) { return tex.name == material.specular_texname; });
				if (texture_pos == loaded_textures.end()) {
					mesh_material.textures.push_back(loadMaterialTexture(material.specular_texname, TAGTexType::SPEC_MAP));
					loaded_textures.push_back(mesh_material.textures.back());
				}
				else {
					TAGTexLoader::Texture texture = *texture_pos;
					texture.type = TAGTexType::SPEC_MAP;
					mesh_material.textures.push_back(texture);
				}
			}

			mesh_material.name = material.name;
			mesh_material.spec_exp = material.shininess;
			mesh_material.spec_fac = material.specular[0];
			mesh_material.opacity = material.dissolve;
			mesh_material.colour = *((glm::vec3*)&material.diffuse);

			mesh_materials.push_back(mesh_material);
		}

		meshes.try_emplace(shape.name, mesh_vertices, mesh_material_frags, mesh_materials);
	}
}

std::array<TAGModel::ShaderObject, 1> TAGModel::shaderConverter(const Object& obj) {
	return { { glm::vec4(obj.position, obj.scale), glm::vec4(obj.rotation_axis, obj.angle) } };
}

const TAGTexLoader::Texture TAGModel::loadMaterialTexture(const std::string& tex_path, const TAGTexType& tex_type) const {
	TAGTexLoader::Texture texture = TAGTexLoader::textureFromFile(this->directory + tex_path, "", this->tex_params);
	texture.type = tex_type;
	return texture;
}

std::vector<std::string> TAGModel::getMeshNames() const {
	std::vector<std::string> names;
	names.reserve(meshes.size());
	for (const auto& pair : meshes) {
		names.push_back(pair.first);
	}
	return names;
}

TAGMesh::DotPlane TAGModel::transformPlane(const Object& obj, const TAGMesh::DotPlane& plane) {
	const glm::vec3 new_normal = glm::mat3(glm::rotate(glm::mat4(1.0f), obj.angle, obj.rotation_axis)) * plane.normal;
	const float new_constant = glm::dot(new_normal, (new_normal * plane.constant * obj.scale) + obj.position);
	return { new_normal, new_constant };
}

TAGMesh::Plane TAGModel::transformPlane(const Object& obj, const TAGMesh::Plane& plane) {
	const glm::mat3 rot_mat = glm::mat3(glm::rotate(glm::mat4(1.0f), obj.angle, obj.rotation_axis));

	return
	{
		.normal = rot_mat * plane.normal,
		.start = (rot_mat * plane.start * obj.scale) + obj.position,
		.axis = { rot_mat * plane.axis[0] * obj.scale, rot_mat * plane.axis[1] * obj.scale }
	};
}

TAGMesh::PlaneVolume TAGModel::transformPlane(const Object& obj, const TAGMesh::PlaneVolume& plane) {
	const glm::mat3 rot_mat = glm::mat3(glm::rotate(glm::mat4(1.0f), obj.angle, obj.rotation_axis));

	const TAGMesh::Plane new_frag_plane = {
		.normal = rot_mat * plane.frag_plane.normal,
		.start = (rot_mat * plane.frag_plane.start * obj.scale) + obj.position,
		.axis = { rot_mat * plane.frag_plane.axis[0] * obj.scale, rot_mat * plane.frag_plane.axis[1] * obj.scale }
	};

	std::array<TAGMesh::DotPlane, 3> new_volume_planes;
	for (size_t i = 0; i < 3; i++) {
		const glm::vec3 new_normal = rot_mat * plane.volume_planes[i].normal;
		const float new_constant = glm::dot(new_normal, (new_normal * plane.volume_planes[i].constant * obj.scale) + obj.position);
		new_volume_planes[i] = { new_normal, new_constant };
	}

	return { new_frag_plane, new_volume_planes };
}

TAGModel::Object TAGModel::pointDirection(const glm::vec3& direction, const glm::vec3& abs_direction) {
	const glm::vec3 cross = glm::cross(abs_direction, direction);
	const float cross_length = glm::length(cross);
	return {
		.rotation_axis = (cross_length > 0.0f ? cross / cross_length : glm::vec3(0.0f, 1.0f, 0.0f)),
		.angle = std::atan2(cross_length, glm::dot(abs_direction, direction))
	};
}
