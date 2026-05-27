#include <ResourceManagerClass.hpp>

void TAGResourceManager::updateAttachedBuffers(const GLuint& vao) {
	if (!vao_binding_indices.contains(vao)) return;

	for (const BindingData& data : vao_binding_indices[vao]) {
		if (data.ptr && data.ptr->isObjectsChanged()) data.ptr->updateBuffer();
	}
}

void TAGResourceManager::updateAttachedBuffers(const ShaderBufferType& buffer_type) {
	if (!shader_binding_indices.contains(buffer_type)) return;

	for (const BindingData& data : shader_binding_indices[buffer_type]) {
		if (data.ptr->isObjectsChanged()) data.ptr->updateBuffer();
	}
}

void TAGResourceManager::updateAttachedBuffers() {
	for (const ShaderBufferType& buffer_type : buffer_types) {
		updateAttachedBuffers(buffer_type);
	}
}

void TAGResourceManager::updateReferencedBuffers(const GLuint& vao) {
	auto map_it = vao_binding_indices.find(vao);

	if (map_it == vao_binding_indices.end()) return;

	for (BindingData& data : map_it->second) {
		if (data.ptr && data.ptr->isObjectsChanged()) data.ptr->updateBuffer();
	}
}

void TAGResourceManager::updateReferencedBuffers(const ShaderBufferType& buffer_type, const TAGShaderManager::Shader& shader) {
	if (!shader.buffer_locations.contains((GLuint) buffer_type) || !shader_binding_indices.contains(buffer_type)) return;

	const std::vector<GLint>& vec = shader.buffer_locations.at((GLuint) buffer_type);
	for (const GLint& index : vec) {
		auto& vec = shader_binding_indices[buffer_type];
		auto it = std::find_if(vec.begin(), vec.end(), [&index](const BindingData& data) { return index == data.binding_index; });
		if (it != vec.end() && it->ptr->isObjectsChanged()) it->ptr->updateBuffer();
	}
}

void TAGResourceManager::updateReferencedBuffers(const TAGShaderManager::Shader& shader) {
	for (const ShaderBufferType& buffer_type : buffer_types) {
		updateReferencedBuffers(buffer_type, shader);
	}
}

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