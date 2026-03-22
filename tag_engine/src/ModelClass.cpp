#include <ModelClass.hpp>

TAGModel::TAGModel(const TAGTexLoader::Params& tex_params, const std::string& path) {
	this->tex_params = tex_params;
	this->directory = TAGResourceManager::asset_path + path.substr(0, path.find_last_of("/") + 1);

	instances.try_emplace("");
	instance_buffers.try_emplace("");

	if (path != "") {
		loadModel(TAGResourceManager::asset_path + path);
	}
}

void TAGModel::drawAll(const TAGShaderManager::Shader& shader, const bool& cull_face, const std::string& mesh_name) {
	if (cull_face) {
		glEnable(GL_CULL_FACE);
	}
	else {
		glDisable(GL_CULL_FACE);
	}

	InstanceDrawBuffer& instance_buffer = this->instance_buffers.at(mesh_name);
	const unsigned int size = (unsigned int)instances.at(mesh_name).size();

	if (instance_buffer.was_updated) {
		std::vector<Object>& instances = this->instances[mesh_name];
		if ((mesh_name == "" ? meshes.at(mesh_draw_order.back()).is_transparent : meshes.at(mesh_name).is_transparent)) {
			std::sort(instances.begin(), instances.end(), 
				[](const Object& a, const Object& b) {
					return TAGUtil::lengthSq(TAGBaseState::camera_position - a.position) > TAGUtil::lengthSq(TAGBaseState::camera_position - b.position);
				}
			);
		}
		std::vector<glm::mat4> mats;
		mats.reserve(size);
		for (const Object& obj : instances) {
			mats.push_back(glm::rotate(glm::translate(glm::mat4(1.0f), obj.position), obj.angle, obj.rotation_axis) * glm::mat4(glm::mat3(obj.scale)));
		}
		if (size > instance_buffer.max_instances) {
			instance_buffer.max_instances = size;
			glNamedBufferData(instance_buffer.vbo, instance_buffer.max_instances * sizeof(glm::mat4), mats.data(), GL_DYNAMIC_DRAW);
		}
		else {
			glNamedBufferSubData(instance_buffer.vbo, 0, size * sizeof(glm::mat4), mats.data());
		}
		instance_buffer.was_updated = false;
	}

	if (mesh_name != "") {
		TAGMesh& mesh = meshes.at(mesh_name);
		glVertexArrayVertexBuffer(mesh.getVAO(), 1, instance_buffer.vbo, 0, sizeof(glm::mat4));
		mesh.drawInstanced(shader, size);
	}
	else {
		for (const std::string& name : mesh_draw_order) {
			TAGMesh& mesh = meshes.at(name);
			const unsigned int& instance_buffer_vbo = (instance_buffer.max_instances == 0 ? instance_buffers.at(name).vbo : instance_buffer.vbo);
			glVertexArrayVertexBuffer(mesh.getVAO(), 1, instance_buffer_vbo, 0, sizeof(glm::mat4));
			mesh.drawInstanced(shader, size);
		}
	}
}

void TAGModel::drawOne(const TAGShaderManager::Shader& shader, const Object& obj, const bool& cull_face, const std::string& mesh_name) {
	if (cull_face) {
		glEnable(GL_CULL_FACE);
	}
	else {
		glDisable(GL_CULL_FACE);
	}

	const glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), obj.position), obj.angle, obj.rotation_axis);
	shader.setMatrix3("normal", glm::mat3(model));
	shader.setMatrix4("model", model * glm::mat4(glm::mat3(obj.scale)));
	if (mesh_name != "") {
		meshes.at(mesh_name).drawUninstanced(shader);
	}
	else {
		for (const std::string& mesh_name : mesh_draw_order) {
			meshes.at(mesh_name).drawUninstanced(shader);
		}
	}
}

void TAGModel::setMaxInstanceCount(const unsigned int& size, const std::string& mesh_name) {
	InstanceDrawBuffer& instance_buffer = instance_buffers.at(mesh_name);
	unsigned int new_vbo = TAGResourceManager::createBuffer<OpenGLBuffer>();

	const GLint new_buffer_size = size * sizeof(glm::mat4);
	glNamedBufferData(new_vbo, new_buffer_size, NULL, GL_DYNAMIC_DRAW);

	GLint current_buffer_size;
	glGetNamedBufferParameteriv(instance_buffer.vbo, GL_BUFFER_SIZE, &current_buffer_size);

	glCopyNamedBufferSubData(
		instance_buffer.vbo,
		new_vbo,
		0, 0,
		glm::min(new_buffer_size, current_buffer_size)
	);

	TAGResourceManager::deleteBuffer<OpenGLBuffer>(instance_buffer.vbo);
	instance_buffer.vbo = new_vbo;
	
	if (mesh_name != "") {
		glVertexArrayVertexBuffer(meshes.at(mesh_name).getVAO(), 1, instance_buffer.vbo, 0, sizeof(glm::mat4));
	}

	instance_buffer.max_instances = size;
}

const std::vector<TAGModel::Object>& TAGModel::getInstances(const std::string& mesh_name) const {
	return instances.at(mesh_name);
}

std::vector<TAGModel::Object>& TAGModel::changeInstances(const std::string& mesh_name) {
	instance_buffers.at(mesh_name).was_updated = true;
	return instances.at(mesh_name);
}

TAGMesh& TAGModel::getMesh(const std::string& mesh_name) {
	return meshes.at(mesh_name);
}

void TAGModel::addMesh(const std::string& mesh_name, const std::vector<TAGMesh::Vertex>& vertices, const std::vector<TAGMesh::Fragment>& frags, const std::vector<TAGMesh::Material>& materials) {
	meshes.try_emplace(mesh_name, vertices, frags, materials);
	instances.try_emplace(mesh_name);
	instance_buffers.try_emplace(mesh_name);


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
	instances.erase(mesh_name);

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

	// Map of already loaded textures
	std::unordered_map<std::string, unsigned int> loaded_textures;

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
				mesh.vertices.emplace_back(all_vertices[current_index.vertex_index], all_normals[glm::abs(current_index.normal_index)], all_texcoords[glm::abs(current_index.texcoord_index)]);
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
				auto texture_pos = loaded_textures.find(material.diffuse_texname);
				if (texture_pos == loaded_textures.end()) {
					mesh_material.textures.push_back(loadMaterialTexture(material.diffuse_texname, TAGTexType::DIFFUSE_MAP));
					loaded_textures.try_emplace(material.diffuse_texname, mesh_material.textures.back().id);
				}
				else {
					mesh_material.textures.emplace_back(texture_pos->second, TAGTexType::DIFFUSE_MAP);
				}
			}

			if (!material.specular_texname.empty()) {
				auto texture_pos = loaded_textures.find(material.specular_texname);
				if (texture_pos == loaded_textures.end()) {
					mesh_material.textures.push_back(loadMaterialTexture(material.specular_texname, TAGTexType::SPEC_MAP));
					loaded_textures.try_emplace(material.specular_texname, mesh_material.textures.back().id);
				}
				else {
					mesh_material.textures.emplace_back(texture_pos->second, TAGTexType::SPEC_MAP);
				}
			}

			mesh_material.spec_exp = material.shininess;
			mesh_material.spec_mod = material.specular[0];
			mesh_material.opacity = material.dissolve;
			if (mesh_material.opacity < 1.0f) {
				mesh.is_transparent = true;
			}
			mesh_material.colour = *((glm::vec3*)&material.diffuse);

			mesh.materials.push_back(mesh_material);
		}

		mesh.setupMesh();

		instances.try_emplace(shape.name);
		instance_buffers.try_emplace(shape.name);

		if (mesh.is_transparent < 1.0f) {
			mesh_draw_order.insert(mesh_draw_order.begin(), shape.name);
		}
		else {
			mesh_draw_order.push_back(shape.name);
		}
	}
}

const TAGMesh::Texture TAGModel::loadMaterialTexture(const std::string& tex_path, const TAGTexType& tex_type) const {
	return TAGMesh::Texture(TAGTexLoader::textureFromFile(this->directory + tex_path, this->tex_params), tex_type);
}

const TAGMesh::Texture TAGModel::loadMaterialTexture(const glm::vec3& vec, const TAGTexType& tex_type) const {
	return TAGMesh::Texture(TAGTexLoader::textureFromColour(vec), tex_type);
}

std::vector<std::string> TAGModel::getMeshNames() const {
	std::vector<std::string> names;
	names.reserve(meshes.size());
	for (const auto& pair : meshes) {
		names.push_back(pair.first);
	}
	return names;
}

TAGModel::ObjectTrans::ObjectTrans(const Object& obj) {
	this->model = glm::rotate(glm::translate(glm::mat4(1.0f), obj.position), obj.angle, obj.rotation_axis);
	this->normal = glm::mat3(this->model);
	this->model *= glm::mat4(glm::mat3(obj.scale));
}

TAGModel::InstanceDrawBuffer::InstanceDrawBuffer() {
	vbo = TAGResourceManager::createBuffer<OpenGLBuffer>();
}

TAGModel::InstanceDrawBuffer::~InstanceDrawBuffer() {
	TAGResourceManager::deleteBuffer<OpenGLBuffer>(vbo);
}