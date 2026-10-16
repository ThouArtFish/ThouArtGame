#include <HUDManagerClass.hpp>

TAGHUDManager::TAGHUDManager(const std::vector<std::string>& paths, const TAGResourceManager::BufferAccess& access, const GLuint& size, const TAGTexLoader::Params& params) : quads(size, [this](const Quad& quad) { return this->shaderConverter(quad); }, access) {
	if (VAO == 0) initMesh();

	for (const std::string& path : paths) loadImage(path, params);
}

TAGHUDManager::TAGHUDManager(const TAGResourceManager::BufferAccess& access, const GLuint& size, const TAGTexLoader::Params& params, const std::string& path) : quads(size, [this](const Quad& quad) { return this->shaderConverter(quad); }, access) {
	if (VAO == 0) initMesh();

	if (path != "") loadImage(path, params);
}

TAGHUDManager::~TAGHUDManager() {
	for (const TAGTexLoader::Texture& tex : images) TAGResourceManager::deleteBuffer<OpenGLObjectType::TextureBuffer>(tex.id);
}

void TAGHUDManager::loadImage(const std::string& path, const TAGTexLoader::Params& params, const std::string& name) {
	const std::string tex_name = (name == "" ? static_cast<std::filesystem::path>(path).stem().string() : name);
	images.push_back(TAGTexLoader::textureFromFile(TAGResourceManager::asset_path + path, tex_name, params));
}

void TAGHUDManager::addImage(const TAGTexLoader::Texture& texture) {
	images.push_back(texture);
}

void TAGHUDManager::deleteImage(const std::string& name, const bool& global_delete) {
	auto pos = std::find_if(images.begin(), images.end(), [&name](const TAGTexLoader::Texture& tex) { return tex.name == name; });

	if (pos == images.end()) return;

	if (global_delete) TAGResourceManager::deleteBuffer<OpenGLObjectType::TextureBuffer>(pos->id);

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

void TAGHUDManager::setLayerVisibility(const GLuint& layer, const TAGEnum& state) {
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
	auto layer_it = std::find_if(layer_vec.begin(), layer_vec.end(), [&quad](const LayerData& layer) { return layer.id == quad.layer; });
	if (layer_it == layer_vec.end()) {
		layer_vec.emplace_back(quad.layer, (GLuint)quads.getAllObjects().size(), 0, false);
		layer_vec = layers.changeObjects();
		std::sort(layer_vec.begin(), layer_vec.end(), [](const LayerData& a, const LayerData& b) { return a.id > b.id; });
		layer_it = std::find_if(layer_vec.begin(), layer_vec.end(), [&quad](const LayerData& layer) { return layer.id == quad.layer; });
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

const std::vector<TAGHUDManager::Quad>& TAGHUDManager::getAllQuads() const {
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
	if (quads.isObjectsChanged()) updateQuadBuffer();

	// Update indirect draw buffer which controls which layer of images are drawn
	if (layers.isObjectsChanged()) layers.updateBuffer();

	// Update buffers attached to shader
	for (const auto& pair : shader.buffer_locations) {
		TAGResourceManager::updateAttachedBuffers((TAGResourceManager::ShaderBufferType)pair.first, pair.second);
	}

	std::array<int, MAX_TEXTURES> texture_indices = {};
	size_t i;
	for (i = 0; i < glm::min(MAX_TEXTURES, used_images.size()); i++) {
		texture_indices[i] = i;
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, used_images[i]);
	}
	glActiveTexture(GL_TEXTURE0);

	shader.set<ShaderUniformType::SINGLE_2D>(texture_array_name, texture_indices[0], i);
	
	glDepthFunc(GL_ALWAYS);
	glBindVertexArray(VAO);
	glBindVertexBuffer(default_vao_instance_binding_point, quads.getBuffer()->getBufferID(), 0, sizeof(ShaderQuad));
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, layers.getBuffer()->getBufferID());
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, (GLsizei) layers.getAllObjects().size(), 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindVertexBuffer(default_vao_instance_binding_point, 0, 0, 0);
	glBindVertexArray(0);
	quads.setFence();
	layers.setFence();
	glDepthFunc(GL_LESS);

	for (const auto& pair : shader.buffer_locations) {
		TAGResourceManager::fenceAttachedBuffers((TAGResourceManager::ShaderBufferType)pair.first, pair.second);
	}
}

void TAGHUDManager::initMesh() {
	VAO = TAGResourceManager::createBuffer<OpenGLObjectType::VertexArrayObject>();
	VBO = TAGResourceManager::createBuffer<OpenGLObjectType::GenericBuffer>();
	EBO = TAGResourceManager::createBuffer<OpenGLObjectType::GenericBuffer>();

	constexpr static std::array<GLfloat, 8> quad_vertices = {
		0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f
	};

	constexpr static std::array<GLuint, 6> quad_indices = {
		0, 1, 3, 1, 2, 3
	};

	GLuint attrib = base_attrib;

	glBindVertexArray(VAO);

	glBindVertexBuffer(0, VBO, 0, sizeof(glm::vec2)); // Base for any image mesh
	glNamedBufferData(VBO, 8 * sizeof(GLfloat), quad_vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // Indices for mesh
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), quad_indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(attrib); // Mesh reader
	glVertexAttribFormat(attrib, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexAttribBinding(attrib++, default_vao_mesh_binding_point);

	// Quad readers
	for (attrib; attrib < base_attrib + 4; attrib++) {
		glEnableVertexAttribArray(attrib);
		if (attrib < base_attrib + 3) {
			glVertexAttribFormat(attrib, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4) * (attrib - base_attrib - 1));
		}
		else {
			glVertexAttribIFormat(attrib, 1, GL_UNSIGNED_INT, sizeof(glm::vec4) * 2);
		}
		glVertexAttribBinding(attrib, default_vao_instance_binding_point);
	}
	glVertexBindingDivisor(1, default_vao_instance_binding_point);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

std::array<OpenGLIndirectCommand, 1> TAGHUDManager::commandConverter(const LayerData& layer_data) {
	const OpenGLIndirectCommand command = { .count = 6, .instance_count = (layer_data.is_hidden ? 0 : layer_data.count), .base_instance = layer_data.start_index };
	return { command };
}

std::array<TAGHUDManager::ShaderQuad, 1> TAGHUDManager::shaderConverter(const Quad& quad) {
	ShaderQuad buffer_quad;
	glm::vec2 data1, data2;

	glm::vec2 dim = (quad.position_format == DimensionFormat::PIXEL ? (glm::vec2)screen_dimensions : glm::vec2(1.0f));
	data1 = ((quad.position * glm::vec2(2.0f, -2.0f)) / dim) + glm::vec2(-1.0f, 1.0f);
	dim = (quad.dimension_format == DimensionFormat::PIXEL ? (glm::vec2)screen_dimensions : glm::vec2(0.5f));
	data2 = quad.dimensions / dim;
	buffer_quad.quad_data = glm::vec4(data1, data2);

	auto tex_pos = std::find_if(images.begin(), images.end(), [&quad](const TAGTexLoader::Texture& tex) { return tex.name == quad.image_name; });
	dim = (quad.texel_format == DimensionFormat::PIXEL && tex_pos != images.end() ? glm::vec2(tex_pos->width, tex_pos->height) : glm::vec2(1.0f));
	data1 = quad.texel_bottom_left / dim;
	data2 = (quad.texel_top_right - quad.texel_bottom_left) / dim;
	buffer_quad.texel_data = glm::vec4(data1, data2);

	auto pos = std::find(used_images.begin(), used_images.end(), tex_pos->id);
	if (pos == used_images.end()) {
		buffer_quad.tex_index = (GLuint)used_images.size() % MAX_TEXTURES;
		used_images.push_back(tex_pos->id);
	}
	else {
		buffer_quad.tex_index = (GLuint)std::distance(used_images.begin(), pos);
	}

	return { buffer_quad };
}
