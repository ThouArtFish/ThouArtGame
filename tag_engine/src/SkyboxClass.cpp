#include <SkyboxClass.hpp>

TAGSkybox::TAGSkybox(const std::vector<std::string>& paths, const TAGTexLoader::Params& params) {
	if (VBO == 0) generateCube();

	std::vector<std::string> total_paths;
	for (const std::string& path : paths) {
		total_paths.push_back(TAGResourceManager::asset_path + path);
	}
	cubemap_ID = TAGTexLoader::cubemapFromMultipleFiles(total_paths, params);
}

TAGSkybox::TAGSkybox(const std::string& path, const TAGTexLoader::Params& params) {
	if (VBO == 0) generateCube();

	// Image data of skybox texture
	const auto image_data = TAGTexLoader::loadRawImageData(TAGResourceManager::asset_path + path, params.flip);
	const auto image_tex = TAGTexLoader::textureFromInfo(image_data, "", params);

	// Size of each face in cubemap
	const GLuint face_size = glm::pow(glm::round(glm::sqrt(image_data.height / 2.0f)), 2);

	// Empty cubemap
	cubemap_ID = TAGTexLoader::emptyCubemap(face_size, image_data.nr_channels);

	// Camera space matrices for each side of the skybox
	static const std::array<glm::mat4, 6> capture_camera = {
	   glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	   glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	   glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	   glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};

	// Framebuffer to capture results from shader
	GLuint capture_FBO = TAGResourceManager::createBuffer<OpenGLObjectType::Framebuffer>();
	GLuint capture_RBO = TAGResourceManager::createBuffer<OpenGLObjectType::Renderbuffer>();
	glNamedRenderbufferStorage(capture_RBO, GL_DEPTH_COMPONENT24, face_size, face_size);
	glNamedFramebufferRenderbuffer(capture_FBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, capture_RBO);

	// Shader for framebuffer capture
	std::string vertex_source, fragment_source;
	vertex_source = fragment_source = TAGShaderManager::shader_version.substr();
	vertex_source += TAGShaderManager::default_source[2].substr();
	fragment_source += TAGShaderManager::default_source[8].substr();
	const TAGShaderManager::Source shader_source = { .vertex = vertex_source, .fragment = fragment_source, .name = "skybox_setup", .is_path = false };
	const TAGShaderManager setup_shader_manager = TAGShaderManager(shader_source);
	const TAGShaderManager::Shader& setup_shader = setup_shader_manager.useShader("skybox_setup");
	setup_shader.set<glm::mat4>("perspective", glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f));
	setup_shader.set<ShaderUniformType::SINGLE_2D>("cubemap", 0);

	// Bind texture 
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, image_tex.id);

	// Should take all sample despite depth
	glDepthFunc(GL_LEQUAL);

	// Change viewport to capture only image
	glViewport(0, 0, face_size, face_size);

	// Bind capture framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, capture_FBO);

	// Render each face
	for (size_t i = 0; i < 6; i++) {
		setup_shader.set<glm::mat4>("view", capture_camera[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap_ID, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDepthFunc(GL_LESS);

	// Clean up
	TAGResourceManager::deleteBuffer<OpenGLObjectType::Framebuffer>(capture_FBO);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::Renderbuffer>(capture_RBO);

	glViewport(0, 0, TAGBaseState::width, TAGBaseState::height);
}

TAGSkybox::~TAGSkybox() {
	TAGResourceManager::deleteBuffer<OpenGLObjectType::TextureBuffer>(cubemap_ID);
}

void TAGSkybox::draw(const TAGShaderManager::Shader& shader, const std::string& cubemap_name) const {
	for (const auto& pair : shader.buffer_locations) {
		TAGResourceManager::updateAttachedBuffers((TAGResourceManager::ShaderBufferType)pair.first, pair.second);
	}

	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);
	glActiveTexture(GL_TEXTURE0);
	shader.set<ShaderUniformType::CUBEMAP>(cubemap_name, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_ID);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

	for (const auto& pair : shader.buffer_locations) {
		TAGResourceManager::fenceAttachedBuffers((TAGResourceManager::ShaderBufferType)pair.first, pair.second);
	}
}

void TAGSkybox::generateCube() {
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(VBO);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(EBO);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::VertexArrayObject>(VAO);

	static constexpr std::array<float, 24> vertices = {
		0.577350269f, 0.577350269f, 0.577350269f,
		-0.577350269f, 0.577350269f, 0.577350269f,
		-0.577350269f, -0.577350269f, 0.577350269f,
		0.577350269f, -0.577350269f, 0.577350269f,
		0.577350269f, 0.577350269f, -0.577350269f,
		-0.577350269f, 0.577350269f, -0.577350269f,
		-0.577350269f, -0.577350269f, -0.577350269f,
		 0.577350269f, -0.57735026f, -0.577350269f
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