#include <ShaderManagerClass.hpp>
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
		GLuint vao_ID;
		glCreateVertexArrays(1, &vao_ID);
		vao_binding_indices[vao_ID] = {};
		return vao_ID;
	case OpenGLObjectType::SHADER_PROGRAM:
		return glCreateProgram();
	case OpenGLObjectType::VERTEX_SHADER:
		return glCreateShader(GL_VERTEX_SHADER);
	case OpenGLObjectType::FRAGMENT_SHADER:
		return glCreateShader(GL_FRAGMENT_SHADER);
	case OpenGLObjectType::TEXTURE_BUFFER:
		GLuint tex_ID;
		glGenTextures(1, &tex_ID);
		return tex_ID;
	default: // GENERIC_BUFFER
		GLuint buf_ID;
		glGenBuffers(1, &buf_ID);
		return buf_ID;
	}
}

void TAGResourceManager::updateAttachedBuffers(const GLuint& vao) {
	if (!vao_binding_indices.contains(vao)) return;

	for (const BindingData& data : vao_binding_indices[vao]) {
		if (data.ptr && data.ptr->isObjectsChanged()) data.ptr->updateBuffer();
	}
}

void TAGResourceManager::updateAttachedBuffers(const ShaderBufferType& buffer_type, const std::vector<int>& buffer_locations) {
	if (!shader_binding_indices.contains((GLuint) buffer_type)) return;

	std::vector<BindingData>& data_vec = shader_binding_indices[(GLuint)buffer_type];
	for (const GLint& index : buffer_locations) {
		auto it = std::find_if(data_vec.begin(), data_vec.end(), [&index](const BindingData& data) { return index == data.binding_index; });
		if (it != data_vec.end() && it->ptr && it->ptr->isObjectsChanged()) it->ptr->updateBuffer();
	}
}

void TAGResourceManager::updateAttachedBuffers(const std::unordered_map<GLuint, std::vector<int>>& buffer_locations) {
	for (const ShaderBufferType& type : buffer_types) {
		if (buffer_locations.contains((GLuint) type)) updateAttachedBuffers(type, buffer_locations.at((GLuint) type));
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