#include <ResourceManagerClass.hpp>

const GLuint& OpenGLHandle::getID() const {
	return ID;
}

VertexArrayObject::VertexArrayObject() {
	glCreateVertexArrays(1, &ID);
}

ProgramShader::ProgramShader() {
	ID = glCreateProgram();
}

VertexShader::VertexShader() {
	ID = glCreateShader(GL_VERTEX_SHADER);
}

FragmentShader::FragmentShader() {
	ID = glCreateShader(GL_FRAGMENT_SHADER);
}

TextureBuffer::TextureBuffer() {
	glGenTextures(1, &ID);
}

GenericBuffer::GenericBuffer() {
	glCreateBuffers(1, &ID);
}

VertexArrayObject::~VertexArrayObject() {
	glDeleteVertexArrays(1, &ID);
}

ProgramShader::~ProgramShader() {
	glDeleteProgram(ID);
}

VertexShader::~VertexShader() {
	glDeleteShader(ID);
}

FragmentShader::~FragmentShader() {
	glDeleteShader(ID);
}

TextureBuffer::~TextureBuffer() {
	glDeleteTextures(1, &ID);
}

GenericBuffer::~GenericBuffer() {
	glDeleteBuffers(1, &ID);
}

void TAGResourceManager::clear() {
	buffers.clear();
}