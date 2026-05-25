#include <HUDManagerClass.hpp>

TAGHUDManager::TAGHUDManager(const std::vector<std::string>& paths, const TAGResourceManager::BufferAccess& access, const GLuint& size, const TAGTexLoader::Params& params) {
	quads = TAGResourceManager::ObjectBuffer<Quad, ShaderQuad>(size, shaderConverter, access);

	if (VAO == 0) {
		initMesh();
	}

	for (const std::string& path : paths) {
		loadImage(path, params);
	}
}

TAGHUDManager::TAGHUDManager(const TAGResourceManager::BufferAccess& access, const GLuint& size, const TAGTexLoader::Params& params, const std::string& path) {
	quads = TAGResourceManager::ObjectBuffer<Quad, ShaderQuad>(size, shaderConverter, access);

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
			TAGResourceManager::deleteBuffer<TAGResourceManager::TextureBuffer>(tex.id);
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

	if (global_delete) TAGResourceManager::deleteBuffer<TAGResourceManager::TextureBuffer>(pos->id);

	images.erase(pos);

	for (int i = 0; i < quads.getAllObjects().size(); i++) {
		if (quads.getObject(i).image_name == name) {
			quads.removeObject(i--);
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
	if (!layers.contains(layer)) {
		layers.emplace(layer, false);
	}
	LayerData& layer_data = layers[layer];
	if ((state == TAGEnum::FALSE || state == TAGEnum::TOGGLE) && !layer_data.is_hidden) {
		layer_data.is_hidden = true;
		for (int i = 0; i < quads.getAllObjects().size(); i++) {
			if (quads.getObject(i).layer == layer) {
				layer_data.hidden_quads.push_back(quads.removeObject(i--));
			}
		}
	}
	else if ((state == TAGEnum::TRUE || state == TAGEnum::TOGGLE) && layer_data.is_hidden) {
		layer_data.is_hidden = false;
		for (const Quad& quad : layer_data.hidden_quads) {
			addQuad(quad);
		}
		layer_data.hidden_quads.clear();
	}
}

void TAGHUDManager::addQuad(const Quad& quad) {
	auto layer_data = layers.find(quad.layer);
	if (layer_data != layers.end() && layer_data->second.is_hidden) {
		layer_data->second.hidden_quads.push_back(quad);
	}
	else if (quads.getAllObjects().empty()) {
		quads.pushObject(quad);
	}
	else {
		auto it = std::find_if(quads.begin(), quads.end(), [&quad](const Quad& other_quad) { return other_quad.layer > quad.layer; });
		if (it == quads.end()) quads.pushObject(quad);
		else quads.insertObject(quad, std::distance(quads.begin(), it));
	}
}

TAGHUDManager::Quad TAGHUDManager::removeQuad(const int& index) {
	return (index < 0 ? quads.popObject() : quads.removeObject(index));
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
	// Bind quad buffer if it is not bound
	quads.getBuffer()->bindToVertexArrayObject(base_attrib + 1, 0, VAO);

	// Update any other buffers referenced by the shader
	TAGResourceManager::updateReferencedBuffers(shader);

	std::array<int, MAX_TEXTURES> texture_indices;
	unsigned int i;
	for (i = 0; i < used_images.size(); i++) {
		texture_indices[i] = i;
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, used_images[i]);
	}
	glActiveTexture(GL_TEXTURE0);

	shader.set<int>(texture_array_name, texture_indices[0], i);

	glDepthFunc(GL_ALWAYS);
	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, quads.getBuffer()->getCurrentObjects());
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

	quads.getBuffer()->setFence();
}

void TAGHUDManager::initMesh() {
	VAO = TAGResourceManager::createBuffer<TAGResourceManager::VertexArrayObject>();
	VBO = TAGResourceManager::createBuffer<TAGResourceManager::GenericBuffer>();
	EBO = TAGResourceManager::createBuffer<TAGResourceManager::GenericBuffer>();

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

	const auto& pos = std::find(used_images.begin(), used_images.end(), tex_pos->id);
	if (pos == used_images.end() && used_images.size() < MAX_TEXTURES) {
		buffer_quad.tex_index = (GLuint)used_images.size();
		used_images.push_back(tex_pos->id);
	}
	else if (pos != used_images.end()) {
		buffer_quad.tex_index = (GLuint)std::distance(used_images.begin(), pos);
	}
	else {
		buffer_quad.tex_index = 0;
	}

	return buffer_quad;
}
