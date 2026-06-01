#include <ResourceManagerClass.hpp>

TAGResourceManager::OpenGLHandle::OpenGLHandle(const OpenGLObjectType& type) : type(type), ID(createObject(type)) {}

TAGResourceManager::OpenGLHandle::~OpenGLHandle() {
	switch (type) {
	case OpenGLObjectType::VERTEX_ARRAY_OBJECT:
		vao_binding_indices.erase(ID);
		glDeleteVertexArrays(1, &ID);
		break;
	case OpenGLObjectType::SHADER_PROGRAM:
		glDeleteProgram(ID);
		break;
	case OpenGLObjectType::GENERIC_BUFFER:
		glDeleteBuffers(1, &ID);
		break;
	case OpenGLObjectType::TEXTURE_BUFFER:
		glDeleteTextures(1, &ID);
		break;
	default: // VERTEX or FRAGMENT SHADER
		glDeleteShader(ID);
	}
}

GLuint TAGResourceManager::OpenGLHandle::createObject(const OpenGLObjectType& type) {
	switch (type) {
	case OpenGLObjectType::VERTEX_ARRAY_OBJECT:
		GLuint ID;
		glCreateVertexArrays(1, &ID);
		vao_binding_indices[ID] = {};
		return ID;
	case OpenGLObjectType::SHADER_PROGRAM:
		return glCreateProgram();
	case OpenGLObjectType::VERTEX_SHADER:
		return glCreateShader(GL_VERTEX_SHADER);
	case OpenGLObjectType::FRAGMENT_SHADER:
		return glCreateShader(GL_FRAGMENT_SHADER);
	case OpenGLObjectType::TEXTURE_BUFFER:
		GLuint ID;
		glGenTextures(1, &ID);
		return ID;
	default: // GENERIC_BUFFER
		GLuint ID;
		glGenBuffers(1, &ID);
		return ID;
	}
}

void TAGResourceManager::updateAttachedBuffers(const GLuint& vao) {
	if (!vao_binding_indices.contains(vao)) return;

	for (const BindingData& data : vao_binding_indices[vao]) {
		if (data.ptr && data.ptr->isObjectsChanged()) data.ptr->updateBuffer();
	}
}

void TAGResourceManager::updateAttachedBuffers(const ShaderBufferType& buffer_type, const TAGShaderManager::Shader& shader) {
	if (!shader.buffer_locations.contains((GLuint)buffer_type) || !shader_binding_indices.contains((GLuint)buffer_type)) return;

	const std::vector<GLint>& index_vec = shader.buffer_locations.at((GLuint)buffer_type);
	std::vector<BindingData>& data_vec = shader_binding_indices[(GLuint)buffer_type];
	for (const GLint& index : index_vec) {
		auto it = std::find_if(data_vec.begin(), data_vec.end(), [&index](const BindingData& data) { return index == data.binding_index; });
		if (it != data_vec.end() && it->ptr && it->ptr->isObjectsChanged()) it->ptr->updateBuffer();
	}
}

void TAGResourceManager::updateAttachedBuffers(const TAGShaderManager::Shader& shader) {
	for (const ShaderBufferType& type : buffer_types) {
		updateAttachedBuffers(type, shader);
	}
}

void TAGResourceManager::deleteBuffer(const GLuint& ID, const OpenGLObjectType& type) {
	const auto it = std::find_if(buffers.begin(), buffers.end(), [&ID, &type](const OpenGLHandle& buf)
		{
			return (type == buf.type && ID == buf.ID);
		}
	);
	if (it != buffers.end()) {
		buffers.erase(it);
	}
}

GLuint TAGResourceManager::createBuffer(const OpenGLObjectType& type) {
	buffers.emplace_back(type);
	return buffers.back().ID;
}

bool TAGResourceManager::isBuffer(const GLuint& ID, const OpenGLObjectType& type) {
	const auto it = std::find_if(buffers.begin(), buffers.end(), [&ID, &type](const OpenGLHandle& buf)
		{
			return (type == buf.type && ID == buf.ID);
		}
	);
	return (it != buffers.end());
}

void TAGResourceManager::clear() {
	buffers.clear();
	vao_binding_indices.clear();
	shader_binding_indices.clear();
}