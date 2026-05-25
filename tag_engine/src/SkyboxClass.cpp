#include <SkyboxClass.hpp>

TAGSkybox::TAGSkybox(const std::string& directory, const TAGTexLoader::Params& params) {
	cubemap_ID = TAGTexLoader::cubemapFromFile(TAGResourceManager::asset_path + directory + (directory.ends_with("/") ? "" : "/"), params);

	static const std::array<float, 24> vertices = {
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f
	};

	static const std::array<unsigned int, 36> indices = {
		0, 3, 1,
		2, 1, 3,
		4, 5, 7,
		6, 7, 5,
		0, 1, 4,
		5, 4, 1,
		3, 7, 2,
		6, 2, 7,
		5, 1, 6,
		2, 6, 1,
		4, 7, 0,
		3, 0, 7
	};

	VBO = TAGResourceManager::createBuffer<TAGResourceManager::GenericBuffer>();
	EBO = TAGResourceManager::createBuffer<TAGResourceManager::GenericBuffer>();
	VAO = TAGResourceManager::createBuffer<TAGResourceManager::VertexArrayObject>();

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

TAGSkybox::~TAGSkybox() {
	if (delete_on_death) {
		TAGResourceManager::deleteBuffer<TAGResourceManager::TextureBuffer>(cubemap_ID);
	}
	TAGResourceManager::deleteBuffer<TAGResourceManager::GenericBuffer>(VBO);
	TAGResourceManager::deleteBuffer<TAGResourceManager::GenericBuffer>(EBO);
	TAGResourceManager::deleteBuffer<TAGResourceManager::VertexArrayObject>(VAO);
}

void TAGSkybox::draw(const TAGShaderManager::Shader& shader, const TAGShaderManager::ShaderOptions& options) const {
	TAGResourceManager::updateReferencedBuffers(shader);

	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glActiveTexture(GL_TEXTURE0);
	shader.set<int>(options.cubemap, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_ID);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}