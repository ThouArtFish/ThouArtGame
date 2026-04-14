#include <HUDManagerClass.hpp>

TAGHUDManager::TAGHUDManager(const std::vector<std::string>& paths, const TAGTexLoader::Params& params) {
	if (VAO == 0) {
		initMesh();
	}

	for (const std::string& path : paths) {
		loadImage(path, params);
	}
}

TAGHUDManager::TAGHUDManager(const TAGTexLoader::Params& params, const std::string& path) {
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
			TAGResourceManager::deleteBuffer<OpenGLTexture>(tex.id);
		}
	}
	glUnmapNamedBuffer(quad_buffer);
	TAGResourceManager::deleteBuffer<OpenGLBuffer>(quad_buffer);
}

void TAGHUDManager::loadImage(const std::string& path, const TAGTexLoader::Params& params, const std::string& name) {
	const std::string tex_name = (name == "" ? static_cast<std::filesystem::path>(path).stem().string() : name);
	images.push_back(TAGTexLoader::textureFromFile(path, params, tex_name));
}

void TAGHUDManager::addImage(const TAGTexLoader::Texture& texture) {
	images.push_back(texture);
}

void TAGHUDManager::deleteImage(const std::string& name, const bool& global_delete) {
	const auto& pos = std::find_if(images.begin(), images.end(), [&name](const TAGTexLoader::Texture& tex) { return tex.name == name; });
	if (pos == images.end()) {
		return;
	}
	if (global_delete) {
		TAGResourceManager::deleteBuffer<OpenGLTexture>(pos->id);
	}
	images.erase(pos);
	const size_t start_quads = quads.size();
	int i = 0;
	while (i < quads.size()) {
		if (quads[i].image_name == name) {
			quads.erase(quads.begin() + i);
			i--;
		}
		i++;
	}
	if (start_quads > i) {
		quads_changed = true;
	}
}

std::vector<TAGHUDManager::Quad>& TAGHUDManager::changeQuads() {
	quads_changed = true;
	return quads;
}

const std::vector<TAGHUDManager::Quad>& TAGHUDManager::getQuads() const {
	return quads;
}

void TAGHUDManager::setLayerVisibility(const unsigned int& layer, const TAGEnum& state) {
	auto pos = std::find_if(layers.begin(), layers.end(),
		[&layer](const LayerData& other_layer) {
			return layer == other_layer.id;
		}
	);
	if (pos == layers.end()) {
		layers.emplace_back(layer, false);
		pos = layers.begin() + layers.size() - 1;
	}
	if (state == TAGEnum::FALSE || (state == TAGEnum::TOGGLE && !pos->is_hidden)) {
		pos->is_hidden = true;
	}
	else if (state == TAGEnum::TRUE || (state == TAGEnum::TOGGLE && pos->is_hidden)) {
		pos->is_hidden = false;
	}
	if (std::count_if(quads.begin(), quads.end(), [&layer](const Quad& quad) { return quad.layer == layer; }) > 0) {
		quads_changed = true;
	}
}

const std::vector<TAGHUDManager::LayerData>& TAGHUDManager::getLayers() const {
	return layers;
}

void TAGHUDManager::setWindowDimensions(const int& width, const int& height) {
	screen_dimensions = { width, height };
	if (std::count_if(quads.begin(), quads.end(), 
		[](const Quad& quad) {
			return (quad.position_format == TAGHUDQuadFormat::PIXEL || quad.dimension_format == TAGHUDQuadFormat::PIXEL); 
		}
	) > 0) {
		quads_changed = true;
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

void TAGHUDManager::updateQuadBuffer() {
	// Create and map quad buffer if not already done
	if (quad_buffer == 0) {
		quad_buffer = TAGResourceManager::createBuffer<OpenGLBuffer>();
		glNamedBufferStorage(quad_buffer, total_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
		quad_buffer_ptr = (BufferQuad*)glMapNamedBufferRange(quad_buffer, 0, total_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	}

	// Set next write region of quad buffer 
	current_fence = (current_fence + 1) % NUM_FENCES;

	// If region is still being used by GPU then wait
	if (fences[current_fence]) {
		while (glClientWaitSync(fences[current_fence], 0, 0) == GL_TIMEOUT_EXPIRED) {
			continue;
		}
		glDeleteSync(fences[current_fence]);
		fences[current_fence] = nullptr;
	}

	// Sort quads client-side by lowest to highest layer
	const unsigned int max_renderable_quads = glm::min(max_quads, (unsigned int)quads.size());
	std::vector<unsigned int> quad_indices;
	quad_indices.reserve(max_renderable_quads);
	for (unsigned int i = 0; i < max_renderable_quads; i++) {
		quad_indices.push_back(i);
	}
	std::sort(quad_indices.begin(), quad_indices.end(), [this](const unsigned int& a, const unsigned int& b) { return this->quads[a].layer >= this->quads[b].layer; });

	// Convert Quads to a simpler structure for reading in shaders
	used_images.clear();
	int skipping = -1;
	std::vector<BufferQuad> buffer_quads;
	buffer_quads.reserve(max_renderable_quads);
	for (const unsigned int& i : quad_indices) {
		const Quad& quad = quads[i];

		if (skipping < 0) {
			const auto& layer = std::find_if(layers.begin(), layers.end(), [&quad](const LayerData& layer) { return layer.id == quad.layer; });
			if (layer == layers.end()) {
				layers.emplace_back(quad.layer, false);
			}
			else if (layer->is_hidden) {
				skipping = layer->id;
			}
		}

		if (skipping >= 0 && quad.layer != skipping) {
			skipping = -1;
		}
		else if (skipping >= 0) {
			continue;
		}

		BufferQuad buffer_quad;
		glm::vec2 dim = (quad.position_format == TAGHUDQuadFormat::PIXEL ? (glm::vec2)screen_dimensions : glm::vec2(1.0f));
		buffer_quad.trans = ((quad.position * glm::vec2(2.0f, -2.0f)) / dim) + glm::vec2(-1.0f, 1.0f);
		dim = (quad.dimension_format == TAGHUDQuadFormat::PIXEL ? (glm::vec2)screen_dimensions : glm::vec2(0.5f));
		buffer_quad.scale = quad.dimensions / dim;

		const auto& tex_pos = std::find_if(images.begin(), images.end(), [&quad](const TAGTexLoader::Texture& tex) { return tex.name == quad.image_name; });
		dim = (quad.texel_format == TAGHUDQuadFormat::PIXEL ? glm::vec2(tex_pos->width, tex_pos->height) : glm::vec2(1.0f));
		buffer_quad.texel_trans = quad.texel_top_left / dim;
		buffer_quad.texel_scale = (quad.texel_bottom_right - quad.texel_top_left) / dim;

		const auto& pos = std::find(used_images.begin(), used_images.end(), tex_pos->id);
		if (pos == used_images.end() && used_images.size() < MAX_TEXTURES) {
			buffer_quad.tex_index = (unsigned int)used_images.size();
			used_images.push_back(tex_pos->id);
		}
		else if (pos != used_images.end()) {
			buffer_quad.tex_index = (unsigned int)std::distance(used_images.begin(), pos);
		}
		else {
			continue;
		}

		buffer_quads.push_back(buffer_quad);
	}

	// Record number of quads to be drawn
	buffer_quad_count = (unsigned int)buffer_quads.size();
	
	// Copy new quads to mapped buffer
	std::memcpy(quad_buffer_ptr + max_quads * current_fence, buffer_quads.data(), buffer_quad_count * sizeof(BufferQuad));

	// Bind new buffer region to vao
	glVertexArrayVertexBuffer(VAO, 1, quad_buffer, (GLintptr)(current_fence * region_size), sizeof(BufferQuad));

	quads_changed = false;
}

void TAGHUDManager::drawAll(const TAGShaderManager::Shader& shader) {
	if (quads_changed) {
		updateQuadBuffer();
	}

	int i;
	std::array<int, MAX_TEXTURES> texture_indices;
	for (i = 0; i < used_images.size(); i++) {
		texture_indices[i] = i;
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, used_images[i]);
	}
	glActiveTexture(GL_TEXTURE0);

	shader.setInt("texture_indices", texture_indices[0], i);

	glDepthFunc(GL_ALWAYS);
	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, buffer_quad_count);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

	fences[current_fence] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void TAGHUDManager::initMesh() {
	VAO = TAGResourceManager::createBuffer<OpenGLVertexArrayObject>();
	VBO = TAGResourceManager::createBuffer<OpenGLBuffer>();
	EBO = TAGResourceManager::createBuffer<OpenGLBuffer>();

	const std::array<float, 8> quad_vertices = {
		0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f
	};

	const std::array<unsigned int, 6> quad_indices = {
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
	for (unsigned int i = 0; i < 5; i++) {
		const unsigned int current_attrib = base_attrib + i + 1;
		glEnableVertexAttribArray(current_attrib);
		if (i < 4) {
			glVertexAttribFormat(current_attrib, 2, GL_FLOAT, GL_FALSE, offsetof(BufferQuad, trans) + sizeof(glm::vec2) * i);
		}
		else {
			glVertexAttribIFormat(current_attrib, 1, GL_UNSIGNED_INT, offsetof(BufferQuad, trans) + sizeof(glm::vec2) * i);
		}
		glVertexAttribBinding(current_attrib, 1);
	}
	glVertexBindingDivisor(1, 1);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
