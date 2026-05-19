#include <ResourceManagerClass.hpp>

const GLuint& TAGResourceManager::OpenGLHandle::getID() const {
	return ID;
}

TAGResourceManager::VertexArrayObject::VertexArrayObject() {
	glCreateVertexArrays(1, &ID);
	vao_binding_indices[ID] = {};
}

TAGResourceManager::ProgramShader::ProgramShader() {
	ID = glCreateProgram();
}

TAGResourceManager::VertexShader::VertexShader() {
	ID = glCreateShader(GL_VERTEX_SHADER);
}

TAGResourceManager::FragmentShader::FragmentShader() {
	ID = glCreateShader(GL_FRAGMENT_SHADER);
}

TAGResourceManager::TextureBuffer::TextureBuffer() {
	glGenTextures(1, &ID);
}

TAGResourceManager::GenericBuffer::GenericBuffer() {
	glCreateBuffers(1, &ID);
}

TAGResourceManager::VertexArrayObject::~VertexArrayObject() {
	vao_binding_indices.erase(ID);
	glDeleteVertexArrays(1, &ID);
}

TAGResourceManager::ProgramShader::~ProgramShader() {
	glDeleteProgram(ID);
}

TAGResourceManager::VertexShader::~VertexShader() {
	glDeleteShader(ID);
}

TAGResourceManager::FragmentShader::~FragmentShader() {
	glDeleteShader(ID);
}

TAGResourceManager::TextureBuffer::~TextureBuffer() {
	glDeleteTextures(1, &ID);
}

TAGResourceManager::GenericBuffer::~GenericBuffer() {
	glDeleteBuffers(1, &ID);
}

void TAGResourceManager::clear() {
	buffers.clear();
	vao_binding_indices.clear();
	shader_binding_indices.clear();
}