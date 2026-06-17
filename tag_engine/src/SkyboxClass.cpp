#include <SkyboxClass.hpp>

TAGSkybox::TAGSkybox(const std::string& directory, const TAGTexLoader::Params& params) {
	cubemap_ID = TAGTexLoader::cubemapFromFile(TAGResourceManager::asset_path + directory + (directory.ends_with("/") ? "" : "/"), params);

	static constexpr std::array<float, 24> vertices = {
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f
	};

	static constexpr std::array<unsigned int, 36> indices = {
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

	VBO = TAGResourceManager::createBuffer<OpenGLObjectType::GenericBuffer>();
	EBO = TAGResourceManager::createBuffer<OpenGLObjectType::GenericBuffer>();
	VAO = TAGResourceManager::createBuffer<OpenGLObjectType::VertexArrayObject>();

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
	TAGResourceManager::deleteBuffer<OpenGLObjectType::TextureBuffer>(cubemap_ID);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(VBO);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(EBO);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::VertexArrayObject>(VAO);
}

void TAGSkybox::draw(const TAGShaderManager::Shader& shader, const std::string& cubemap_name) const {
	TAGResourceManager::updateAttachedBuffers(shader.buffer_locations);

	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glActiveTexture(GL_TEXTURE0);
	shader.set<ShaderUniformType::CUBEMAP>(cubemap_name, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_ID);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}