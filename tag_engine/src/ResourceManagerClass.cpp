#include <ResourceManagerClass.hpp>

OpenGLVertexArrayObject::OpenGLVertexArrayObject() {
	glCreateVertexArrays(1, &ID);
}

OpenGLProgram::OpenGLProgram() {
	ID = glCreateProgram();
}

OpenGLVertexShader::OpenGLVertexShader() {
	ID = glCreateShader(GL_VERTEX_SHADER);
}

OpenGLFragmentShader::OpenGLFragmentShader() {
	ID = glCreateShader(GL_FRAGMENT_SHADER);
}

OpenGLTexture::OpenGLTexture() {
	glGenTextures(1, &ID);
}

OpenGLBuffer::OpenGLBuffer() {
	glCreateBuffers(1, &ID);
}

OpenGLVertexArrayObject::~OpenGLVertexArrayObject() {
	glDeleteVertexArrays(1, &ID);
}

OpenGLProgram::~OpenGLProgram() {
	glDeleteProgram(ID);
}

OpenGLVertexShader::~OpenGLVertexShader() {
	glDeleteShader(ID);
}

OpenGLFragmentShader::~OpenGLFragmentShader() {
	glDeleteShader(ID);
}

OpenGLTexture::~OpenGLTexture() {
	glDeleteTextures(1, &ID);
}

OpenGLBuffer::~OpenGLBuffer() {
	glDeleteBuffers(1, &ID);
}

void TAGResourceManager::clear() {
	buffers.clear();
}