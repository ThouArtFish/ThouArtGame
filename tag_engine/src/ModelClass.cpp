#include <ModelClass.hpp>

TAGModel::TAGModel(const TAGTexLoader::Params& tex_params, const TAGResourceManager::BufferAccess& access, const std::string& path) {
	this->tex_params = tex_params;
	this->access = access;
	directory = TAGResourceManager::asset_path + path.substr(0, path.find_last_of("/") + 1);

	if (path != "") loadModel(TAGResourceManager::asset_path + path);
}

void TAGModel::drawAll(const TAGShaderManager::Shader& shader, const std::string& mesh_name, const TAGShaderManager::ShaderOptions& options) {
	if ((mesh_name != "" && meshes.find(mesh_name) == meshes.end()) || instance_buffers.find(mesh_name) == instance_buffers.end()) return;

	TAGResourceManager::ObjectBuffer<Object, ShaderObject>& instance_buffer = instance_buffers.at(mesh_name);

	TAGResourceManager::updateAttachedBuffers(shader.buffer_locations);

	if (!options.cull_backface) glDisable(GL_CULL_FACE);

	if (mesh_name != "") {
		TAGMesh& mesh = meshes.at(mesh_name);

		if (instance_buffer.isObjectsChanged()) updateInstanceBuffer(mesh_name);

		instance_buffer.bindToVertexArrayObject(default_vao_instance_binding_point, 0, mesh.getVAO());

		mesh.draw(shader, options, instance_buffer.getBuffer()->getCurrentObjects());
	}
	else {
		for (const std::string& name : mesh_draw_order) {
			TAGMesh& mesh = meshes.at(name);

			if (instance_buffer.isObjectsChanged()) updateInstanceBuffer(mesh_name);

			instance_buffer.bindToVertexArrayObject(default_vao_instance_binding_point, 0, mesh.getVAO());

			mesh.draw(shader, options, instance_buffer.getBuffer()->getCurrentObjects());
		}
	}
	instance_buffer.getBuffer()->setFence();

	if (!options.cull_backface) glEnable(GL_CULL_FACE);
}

void TAGModel::drawOne(const TAGShaderManager::Shader& shader, const Object& obj, const std::string& mesh_name, const TAGShaderManager::ShaderOptions& options) {
	if (mesh_name != "" && meshes.find(mesh_name) == meshes.end()) return;

	TAGResourceManager::updateAttachedBuffers(shader.buffer_locations);

	if (!options.cull_backface) glDisable(GL_CULL_FACE);
	
	const std::array<glm::vec4, 2> shader_object = { glm::vec4(obj.position, obj.scale), glm::vec4(obj.rotation_axis, obj.angle) };
	shader.set<glm::vec4>(options.shader_object, shader_object[0], 2);
	if (mesh_name != "") {
		meshes.at(mesh_name).draw(shader, options);
	}
	else {
		for (const std::string& mesh_name : mesh_draw_order) {
			meshes.at(mesh_name).draw(shader, options);
		}
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
	auto& instance_buffer = instance_buffers.at(mesh_name);
	std::vector<Object>& object_vec = instance_buffer.changeObjects();
	if (mesh_name == "" ? meshes.at(mesh_draw_order.back()).is_transparent : meshes.at(mesh_name).is_transparent) {
		std::sort(object_vec.begin(), object_vec.end(),
			[](const Object& a, const Object& b) {
				return TAGUtil::lengthSq(TAGBaseState::camera_position - a.position) > TAGUtil::lengthSq(TAGBaseState::camera_position - b.position);
			}
		);
	}
	instance_buffer.updateBuffer();
}

TAGMesh& TAGModel::getMesh(const std::string& mesh_name) {
	return meshes.at(mesh_name);
}

void TAGModel::addMesh(const std::string& mesh_name, const std::vector<TAGMesh::Vertex>& vertices, const std::vector<TAGMesh::Fragment>& frags, const std::vector<TAGMesh::Material>& materials) {
	meshes.try_emplace(mesh_name, vertices, frags, materials);

	if (meshes[mesh_name].is_transparent) {
		mesh_draw_order.insert(mesh_draw_order.begin(), mesh_name);
	}
	else {
		mesh_draw_order.push_back(mesh_name);
	}
}

void TAGModel::deleteMesh(const std::string& mesh_name) {
	meshes.erase(mesh_name);
	instance_buffers.erase(mesh_name);
	mesh_draw_order.erase(std::find(mesh_draw_order.begin(), mesh_draw_order.end(), mesh_name));
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
		std::cout << "TINYOBJLOADER::WARN: " << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << "TINYOBJLOADER::ERR: " << err << std::endl;
	}

	if (!ret) {
		std::cout << "Failed to parse model" << std::endl;
		return;
	}

	// Get position, normal and tex coords in vector format
	const std::array<size_t, 3> array_sizes = { attrib.vertices.size() / 3, attrib.normals.size() / 3, attrib.texcoords.size() / 2 };
	std::vector<glm::vec3> all_vertices;
	all_vertices.assign(array_sizes[0], glm::vec3(0));
	std::vector<glm::vec3> all_normals;
	all_normals.assign(array_sizes[1], glm::normalize(glm::vec3(1)));
	std::vector<glm::vec2> all_texcoords;
	all_texcoords.assign(array_sizes[2], glm::vec2(0));
	for (size_t i = 0; i < glm::max(glm::max(array_sizes[0], array_sizes[1]), array_sizes[2]); i++) {
		if (i < array_sizes[0]) {
			for (unsigned int j = 0; j < 3; j++) {
				all_vertices[i][j] = attrib.vertices[i * 3 + j];
			}
		}
		if (i < array_sizes[1]) {
			for (unsigned int j = 0; j < 3; j++) {
				all_normals[i][j] = attrib.normals[i * 3 + j];
			}
			all_normals[i] = glm::normalize(all_normals[i]);
		}
		if (i < array_sizes[2]) {
			for (unsigned int j = 0; j < 2; j++) {
				all_texcoords[i][j] = attrib.texcoords[i * 2 + j];
			}
		}
	}

	// Function for checking if a vertex has already been added to a meshes vertex array
	tinyobj::index_t current_index;
	auto checkSameFrag = [&current_index](const tinyobj::index_t& index) {
		return (current_index.vertex_index == index.vertex_index && current_index.normal_index == index.normal_index && current_index.texcoord_index == index.texcoord_index);
	};

	// Loaded textures
	std::vector<TAGTexLoader::Texture> loaded_textures;

	// Load each mesh
	for (const tinyobj::shape_t& shape : shapes) {
		meshes.try_emplace(shape.name);
		TAGMesh& mesh = meshes.at(shape.name);

		mesh.vertices.reserve(shape.mesh.indices.size());
		mesh.frags.reserve(shape.mesh.num_face_vertices.size());

		// Indices of materials used by current mesh
		std::vector<unsigned int> found_materials;

		// Vertices which are part of mesh already
		std::vector<tinyobj::index_t> unique_indices;

		// Current primitive
		std::array<unsigned int, 3> primitive;

		// Next free index in primitive
		size_t primitive_index = 0;

		// Loop through each vertex in mesh
		for (size_t i = 0; i < shape.mesh.indices.size(); i++) {
			current_index = shape.mesh.indices[i];
			const auto& vertex_pos = std::find_if(unique_indices.begin(), unique_indices.end(), checkSameFrag);
			if (vertex_pos == unique_indices.end()) {
				primitive[primitive_index] = (unsigned int)mesh.vertices.size();
				mesh.vertices.emplace_back(all_vertices[current_index.vertex_index], all_normals[current_index.normal_index], all_texcoords[current_index.texcoord_index]);
				unique_indices.push_back(current_index);
			}
			else {
				primitive[primitive_index] = (unsigned int)std::distance(unique_indices.begin(), vertex_pos);
			}

			if (primitive_index == 2) {
				unsigned int material_index = shape.mesh.material_ids[(i - 2) / 3];
				const auto& material_pos = std::find(found_materials.begin(), found_materials.end(), material_index);
				if (material_pos == found_materials.end()) {
					found_materials.push_back(material_index);
					material_index = (unsigned int)(found_materials.size() - 1);
				}
				else {
					material_index = (unsigned int)std::distance(found_materials.begin(), material_pos);
				}
				mesh.frags.emplace_back(primitive, material_index);
				primitive_index = 0;
			}
			else {
				primitive_index++;
			}
		}

		mesh.vertices.shrink_to_fit();
		mesh.frags.shrink_to_fit();

		// Load materials for model
		for (const unsigned int& material_index : found_materials) {
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
			if (mesh_material.opacity < 1.0f) {
				mesh.is_transparent = true;
			}
			mesh_material.colour = *((glm::vec3*)&material.diffuse);

			mesh.materials.push_back(mesh_material);
		}

		mesh.setupMesh();

		if (mesh.is_transparent) {
			mesh_draw_order.insert(mesh_draw_order.begin(), shape.name);
		}
		else {
			mesh_draw_order.push_back(shape.name);
		}
	}
}

TAGModel::ShaderObject TAGModel::shaderConverter(const Object& obj, const GLuint& split) {
	return { glm::vec4(obj.position, obj.scale), glm::vec4(obj.rotation_axis, obj.angle) };
}

const TAGTexLoader::Texture TAGModel::loadMaterialTexture(const std::string& tex_path, const TAGTexType& tex_type) const {
	TAGTexLoader::Texture texture = TAGTexLoader::textureFromFile(this->directory + tex_path, this->tex_params);
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
