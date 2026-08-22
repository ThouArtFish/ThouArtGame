#include <ResourceManagerClass.hpp>

TAGResourceManager::OpenGLHandleWrapper::OpenGLHandleWrapper(const GLuint& ID, const GLuint& TYPE_ID) : ID(ID), TYPE_ID(TYPE_ID) {}

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

void TAGResourceManager::fenceAttachedBuffers(const GLuint& vao) {
	if (!vao_binding_indices.contains(vao)) return;

	for (const BindingData& data : vao_binding_indices[vao]) {
		if (data.ptr && data.ptr->isObjectsChanged()) data.ptr->setFence();
	}
}

void TAGResourceManager::fenceAttachedBuffers(const ShaderBufferType& buffer_type, const std::vector<int>& buffer_locations) {
	if (!shader_binding_indices.contains((GLuint)buffer_type)) return;

	std::vector<BindingData>& data_vec = shader_binding_indices[(GLuint)buffer_type];
	for (const GLint& index : buffer_locations) {
		auto it = std::find_if(data_vec.begin(), data_vec.end(), [&index](const BindingData& data) { return index == data.binding_index; });
		if (it != data_vec.end() && it->ptr) it->ptr->setFence();
	}
}

void TAGResourceManager::clear() {
	buffers.clear();
	vao_binding_indices.clear();
	shader_binding_indices.clear();
}