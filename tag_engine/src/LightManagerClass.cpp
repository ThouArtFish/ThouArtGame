#include <LightManagerClass.hpp>

TAGLightManager::TAGLightManager(const unsigned int& size, const TAGLightManager::ChangeFreq& changeFreq) : buffer_size(size) {
	lights.reserve(buffer_size);
	buffer_ID = TAGResourceManager::createBuffer<OpenGLBuffer>();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_ID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Light) * buffer_size + sizeof(glm::vec4), NULL, (GLenum)changeFreq);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4), (void*)0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

TAGLightManager::TAGLightManager(const std::vector<Light>& lights, const TAGLightManager::ChangeFreq& changeFreq) : buffer_size((unsigned int)lights.size()) {
	this->lights = lights;
	buffer_ID = TAGResourceManager::createBuffer<OpenGLBuffer>();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_ID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Light) * buffer_size + sizeof(glm::vec4), NULL, (GLenum)changeFreq);
	const size_t size = lights.size();
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4), &size);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4), sizeof(Light) * buffer_size, lights.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

TAGLightManager::~TAGLightManager() {
	if (delete_on_death) {
		TAGResourceManager::deleteBuffer<OpenGLBuffer>(buffer_ID);
	}
}

std::vector<TAGLightManager::Light>& TAGLightManager::changeLights() {
	was_updated = true;
	return lights;
}

const std::vector<TAGLightManager::Light>& TAGLightManager::getLights() const {
	return lights;
}

void TAGLightManager::bindShaderData(const unsigned int& index) {
	if (was_updated) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_ID);
		if (lights.size() > buffer_size) {
			buffer_size = (unsigned int)lights.size();
			glBufferData(GL_SHADER_STORAGE_BUFFER, buffer_size * sizeof(Light) + sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
		}
		const size_t size = lights.size();
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4), &size);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4), buffer_size * sizeof(Light), lights.data());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		was_updated = false;
	}
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer_ID);
}

void TAGLightManager::unbindShaderData(const unsigned int& index) const {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, 0);
}

auto TAGLightManager::begin() const {
	return lights.begin();
}

auto TAGLightManager::end() const {
	return lights.end();
}

unsigned int TAGLightManager::size() const {
	return (unsigned int)lights.size();
}