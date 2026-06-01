#include <HUDManagerClass.hpp>

TAGHUDManager::TAGHUDManager(const std::vector<std::string>& paths, const TAGResourceManager::BufferAccess& access, const GLuint& size, const TAGTexLoader::Params& params) : quads(size, [this](const Quad& quad, const GLuint& split) { return this->shaderConverter(quad, split); }, access) {
	if (VAO == 0) {
		initMesh();
	}

	for (const std::string& path : paths) {
		loadImage(path, params);
	}
}

TAGHUDManager::TAGHUDManager(const TAGResourceManager::BufferAccess& access, const GLuint& size, const TAGTexLoader::Params& params, const std::string& path) : quads(size, [this](const Quad& quad, const GLuint& split) { return this->shaderConverter(quad, split); }, access) {
	if (VAO == 0) {
		initMesh();
	}

	if (path != "") {
		loadImage(path, params);
	}
}

TAGHUDManager::~TAGHUDManager() {
	if (delete_on_death) {
		for (const TAGTexLoader::Texture& tex : images) {
			TAGResourceManager::deleteBuffer(tex.id, TAGResourceManager::OpenGLObjectType::TEXTURE_BUFFER);
		}
	}
}

void TAGHUDManager::loadImage(const std::string& path, const TAGTexLoader::Params& params, const std::string& name) {
	const std::string tex_name = (name == "" ? static_cast<std::filesystem::path>(path).stem().string() : name);
	images.push_back(TAGTexLoader::textureFromFile(path, params, tex_name));
}

void TAGHUDManager::addImage(const TAGTexLoader::Texture& texture) {
	images.push_back(texture);
}

void TAGHUDManager::deleteImage(const std::string& name, const bool& global_delete) {
	auto pos = std::find_if(images.begin(), images.end(), [&name](const TAGTexLoader::Texture& tex) { return tex.name == name; });

	if (pos == images.end()) return;

	if (global_delete) TAGResourceManager::deleteBuffer(pos->id, TAGResourceManager::OpenGLObjectType::TEXTURE_BUFFER);

	images.erase(pos);

	std::unordered_map<GLuint, GLuint> layer_removed_count;
	for (int i = 0; i < quads.getAllObjects().size(); i++) {
		if (quads.getObject(i).image_name == name) {
			const Quad quad = quads.removeObject(i--);
			if (layer_removed_count.contains(quad.layer)) {
				layer_removed_count[quad.layer]++;
			}
			else {
				layer_removed_count[quad.layer] = 1;
			}
		}
	}

	if (!layer_removed_count.empty()) {
		const std::vector<Quad>& quad_vec = quads.getAllObjects();
		for (int i = 0; i < layers.getAllObjects().size(); i++) {
			LayerData layer_data = layers.getObject(i);
			auto it = std::find_if(quad_vec.begin(), quad_vec.end(), [&layer_data](const Quad& quad) { return quad.layer == layer_data.id; });
			layer_data.start_index = (GLuint) std::distance(quad_vec.begin(), it);
			layer_data.count -= (layer_removed_count.contains(layer_data.id) ? layer_removed_count[layer_data.id] : 0);
			if (layer_data.count == 0) {
				layers.removeObject(i--);
			}
			else {
				layers.setObject(layer_data, i);
			}
		}
	}
}

std::vector<std::string> TAGHUDManager::getImageNames() const {
	std::vector<std::string> names;
	names.reserve(images.size());
	for (const TAGTexLoader::Texture& tex : images) {
		names.push_back(tex.name);
	}
	return names;
}

void TAGHUDManager::setLayerVisibility(const unsigned int& layer, const TAGEnum& state) {
	auto vec_it = std::find_if(layers.begin(), layers.end(), [&layer](const LayerData& layer_data) { return layer == layer_data.id; });
	if (vec_it == layers.end() || (vec_it->is_hidden && state == TAGEnum::FALSE) || (!vec_it->is_hidden && state == TAGEnum::TRUE)) return;

	const GLuint layer_index = (GLuint) std::distance(layers.begin(), vec_it);
	std::vector<LayerData>& layer_vec = layers.changeObjects();
	if ((state == TAGEnum::FALSE || state == TAGEnum::TOGGLE) && !layer_vec[layer_index].is_hidden) {
		layer_vec[layer_index].is_hidden = true;
	}
	else if ((state == TAGEnum::TRUE || state == TAGEnum::TOGGLE) && layer_vec[layer_index].is_hidden) {
		layer_vec[layer_index].is_hidden = false;
	}
}

void TAGHUDManager::addQuad(const Quad& quad) {
	std::vector<LayerData>& layer_vec = layers.changeObjects();
	auto layer_it = std::find_if(layer_vec.begin(), layer_vec.end(), [&quad](const LayerData& layer) { return quad.layer == layer.id; });
	if (layer_it == layer_vec.end()) {
		layer_vec.emplace_back(quad.layer, 0, quads.getAllObjects().size(), false);
		layer_vec = layers.changeObjects();
		layer_it = layer_vec.begin() + layer_vec.size() - 1;
	}
	
	layer_it->count++;
	auto quad_it = std::find_if(quads.begin(), quads.end(), [&quad](const Quad& other_quad) { return other_quad.layer <= quad.layer; });
	if (quad_it == quads.end()) { 
		quads.pushObject(quad); 
	}
	else { 
		layer_it->start_index = (GLuint) std::distance(quads.begin(), quad_it);
		quads.insertObject(quad, layer_it->start_index);
		for (LayerData& layer_data : layer_vec) {
			if (layer_data.id < quad.layer) {
				layer_data.start_index++;
			}
		}
	}
}

void TAGHUDManager::setQuad(const Quad& quad, const GLuint& index) {
	Quad other_quad = quads.getObject(index);
	quads.setObject(quad, index);
	quads.changeObjects()[index].layer = other_quad.layer;
}

TAGHUDManager::Quad TAGHUDManager::removeQuad(const int& index) {
	Quad quad = (index < 0 ? quads.popObject() : quads.removeObject(index));
	std::vector<LayerData>& layer_vec = layers.changeObjects();
	auto layer_it = std::find_if(layer_vec.begin(), layer_vec.end(), [&quad](const LayerData& layer) { return quad.layer == layer.id; });
	if (--layer_it->count == 0) {
		layer_vec.erase(layer_it);
		layer_vec = layers.changeObjects();
	}
	for (LayerData& layer_data : layer_vec) {
		if (layer_data.id < quad.layer) {
			layer_data.start_index--;
		}
	}
	return quad;
}

const TAGHUDManager::Quad& TAGHUDManager::getQuad(const int& index) const {
	return (index < 0 ? quads.peekObject() : quads.getObject(index));
}

const std::vector<TAGHUDManager::Quad>& TAGHUDManager::getAllLights() const {
	return quads.getAllObjects();
}

void TAGHUDManager::setWindowDimensions(const int& width, const int& height) {
	screen_dimensions = { width, height };
	for (int i = 0; i < quads.getAllObjects().size(); i++) {
		const Quad& quad = quads.getObject(i);
		if (quad.position_format == DimensionFormat::RELATIVE || quad.dimension_format == DimensionFormat::RELATIVE) {
			updateQuadBuffer();
			break;
		}
	}
}

void TAGHUDManager::updateQuadBuffer() {
	used_images.clear();
	quads.updateBuffer();
}

void TAGHUDManager::drawAll(const TAGShaderManager::Shader& shader, const std::string& texture_array_name) {
	// Update quad buffer if any changes
	if (quads.isObjectsChanged()) quads.updateBuffer();

	// Bind quad buffer if it is not bound
	quads.getBuffer()->bindToVertexArrayObject(base_attrib + 1, 0, VAO);

	// Update any other buffers referenced by the shader
	TAGResourceManager::updateAttachedBuffers(shader);

	// Update indirect draw buffer which controls which layer of images are drawn
	if (layers.isObjectsChanged()) layers.updateBuffer();

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, layers.getBuffer()->getBufferID());

	std::array<int, MAX_TEXTURES> texture_indices;
	unsigned int i;
	for (i = 0; i < glm::min(MAX_TEXTURES, (GLuint) used_images.size()); i++) {
		texture_indices[i] = i;
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, used_images[i]);
	}
	glActiveTexture(GL_TEXTURE0);

	shader.set<int>(texture_array_name, texture_indices[0], i);

	glDepthFunc(GL_ALWAYS);
	glBindVertexArray(VAO);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, (GLsizei) layers.getAllObjects().size(), sizeof(OpenGLIndirectCommand));
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	quads.getBuffer()->setFence();
}

void TAGHUDManager::initMesh() {
	VAO = TAGResourceManager::createBuffer(TAGResourceManager::OpenGLObjectType::VERTEX_ARRAY_OBJECT);
	VBO = TAGResourceManager::createBuffer(TAGResourceManager::OpenGLObjectType::GENERIC_BUFFER);
	EBO = TAGResourceManager::createBuffer(TAGResourceManager::OpenGLObjectType::GENERIC_BUFFER);

	const static std::array<float, 8> quad_vertices = {
		0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f
	};

	const static std::array<unsigned int, 6> quad_indices = {
		0, 1, 3, 1, 2, 3
	};

	glBindVertexArray(VAO);

	glBindVertexBuffer(0, VBO, 0, sizeof(glm::vec2)); // Base for any image mesh
	glNamedBufferData(VBO, 8 * sizeof(float), quad_vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // Indices for mesh
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), quad_indices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(base_attrib); // Mesh reader
	glVertexAttribFormat(base_attrib, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexAttribBinding(base_attrib, 0);

	// Quad readers
	for (unsigned int i = 0; i < 3; i++) {
		const unsigned int current_attrib = base_attrib + 1 + i;
		glEnableVertexAttribArray(current_attrib);
		if (i < 2) {
			glVertexAttribFormat(current_attrib, 4, GL_FLOAT, GL_FALSE, offsetof(ShaderQuad, quad_data) + sizeof(glm::vec4) * i);
		}
		else {
			glVertexAttribIFormat(current_attrib, 1, GL_UNSIGNED_INT, offsetof(ShaderQuad, quad_data) + sizeof(glm::vec4) * i);
		}
		glVertexAttribBinding(current_attrib, 1);
	}
	glVertexBindingDivisor(1, 1);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

OpenGLIndirectCommand TAGHUDManager::commandConverter(const LayerData& layer_data, const GLuint& split) {
	return { .count = 6, .instance_count = (layer_data.is_hidden ? 0 : layer_data.count), .base_instance = layer_data.start_index };
}

TAGHUDManager::ShaderQuad TAGHUDManager::shaderConverter(const Quad& quad, const GLuint& split) {
	ShaderQuad buffer_quad;
	glm::vec2 data1, data2;

	glm::vec2 dim = (quad.position_format == DimensionFormat::PIXEL ? (glm::vec2)screen_dimensions : glm::vec2(1.0f));
	data1 = ((quad.position * glm::vec2(2.0f, -2.0f)) / dim) + glm::vec2(-1.0f, 1.0f);
	dim = (quad.dimension_format == DimensionFormat::PIXEL ? (glm::vec2)screen_dimensions : glm::vec2(0.5f));
	data2 = quad.dimensions / dim;
	buffer_quad.quad_data = glm::vec4(data1, data2);

	auto tex_pos = std::find_if(images.begin(), images.end(), [&quad](const TAGTexLoader::Texture& tex) { return tex.name == quad.image_name; });
	dim = (quad.texel_format == DimensionFormat::PIXEL ? glm::vec2(tex_pos->width, tex_pos->height) : glm::vec2(1.0f));
	data1 = quad.texel_top_left / dim;
	data2 = (quad.texel_bottom_right - quad.texel_top_left) / dim;
	buffer_quad.texel_data = glm::vec4(data1, data2);

	auto pos = std::find(used_images.begin(), used_images.end(), tex_pos->id);
	if (pos == used_images.end()) {
		buffer_quad.tex_index = (GLuint)used_images.size() % MAX_TEXTURES;
		used_images.push_back(tex_pos->id);
	}
	else {
		buffer_quad.tex_index = (GLuint)std::distance(used_images.begin(), pos);
	}

	return buffer_quad;
}
