#pragma once

#include <ResourceManagerClass.hpp>

template<class T> unsigned int TAGResourceManager::BufferHandler<T>::getMaxObjects() const {
	return max_objs;
}

template<class T> unsigned int TAGResourceManager::BufferHandler<T>::getCurrentObjects() const {
	return current_objs;
}

template<class T> void TAGResourceManager::BufferHandler<T>::bindToShader(const GLuint& binding_index, const GLintptr& offset, const ShaderBuffer& buffer_option) {
	glBindBufferRange((GLenum) buffer_option, binding_index, buffer_id, internal_offset + offset, current_objs * sizeof(T));
	if (std::find(bound_buffers.begin(), bound_buffers.end(), buffer_option) == bound_buffers.end()) {
		bound_buffers.push_back(buffer_option);
	}
	std::vector<BindingData>& bindings = shader_binding_indices[buffer_option];
	auto it = std::find_if(bindings.begin(), bindings.end(), [&binding_index](const BindingData& data) { return data.binding_index == binding_index; });
	if (it == bindings.end()) {
		bindings.emplace_back(binding_index, buffer_id, offset);
	}
	else {
		bindings.emplace(it, binding_index, buffer_id, offset);
	}
}

template<class T> void TAGResourceManager::BufferHandler<T>::bindToVertexArrayObject(const GLuint& binding_index, const GLintptr& offset, const GLuint& vao) {
	auto map_it = vao_binding_indices.find(vao);
	if (map_it != vao_binding_indices.end()) {
		glVertexArrayVertexBuffer(vao, binding_index, buffer_id, internal_offset + offset, sizeof(T));
		if (std::find(bound_vaos.begin(), bound_vaos.end(), vao) == bound_vaos.end()) {
			bound_vaos.push_back(vao);
		}
		auto vec_it = std::find_if(map_it->second.begin(), map_it->second.end(), [&binding_index](const BindingData& data) { return binding_index == data.binding_index; });
		if (vec_it == map_it->second.end()) {
			map_it->second.emplace_back(binding_index, buffer_id, offset);
		}
		else {
			map_it->second.emplace(vec_it, binding_index, buffer_id, offset);
		}
	}
}

template<class T> void TAGResourceManager::BufferHandler<T>::setFence() {
	fences[current_fence] = { .sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0) };
}

template<class T> void TAGResourceManager::BufferHandler<T>::updateBindings(const GLuint& new_buffer_id) {
	int i;
	for (i = 0; i < bound_vaos.size(); i++) {
		auto map_it = vao_binding_indices.find(bound_vaos[i]);
		if (map_it == vao_binding_indices.end() || map_it->second.empty()) {
			bound_vaos.erase(bound_vaos.begin() + i--);
			continue;
		}
		bool contains_old_id = false;
		for (BindingData& binding_data : map_it->second) {
			if (binding_data.buffer_id == buffer_id) {
				glVertexArrayVertexBuffer(map_it->first, binding_data.binding_index, new_buffer_id, binding_data.offset, sizeof(T));
				binding_data.buffer_id = new_buffer_id;
				contains_old_id = true;
			}
		};
		if (!contains_old_id) {
			bound_vaos.erase(bound_vaos.begin() + i--);
		}
	}
	for (i = 0; i < bound_buffers.size(); i++) {
		auto map_it = shader_binding_indices.find(bound_buffers[i]);
		if (map_it == shader_binding_indices.end() || map_it->second.empty()) {
			bound_buffers.erase(bound_buffers.begin() + i--);
			continue;
		}
		bool contains_old_id = false;
		for (BindingData& binding_data : map_it->second) {
			if (binding_data.buffer_id == buffer_id) {
				glBindBufferRange((GLenum)map_it->first, binding_data.binding_index, new_buffer_id, binding_data.offset, current_objs * sizeof(T));
				binding_data.buffer_id = new_buffer_id;
				contains_old_id = true;
			}
		};
		if (!contains_old_id) {
			bound_buffers.erase(bound_buffers.begin() + i--);
		}
	}
}

template<class T, unsigned int MAX_FENCES> TAGResourceManager::RingBuffer<T, MAX_FENCES>::RingBuffer(const unsigned int& max_objs) {
	this->max_objs = max_objs;
	fences = std::make_unique<GLsyncWrap[]>(MAX_FENCES);

	const unsigned int total_size = max_objs * sizeof(T) * MAX_FENCES;
	buffer_id = createBuffer<GenericBuffer>();
	glNamedBufferStorage(buffer_id, total_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	buffer_ptr = (T*)glMapNamedBufferRange(buffer_id, 0, total_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

template<class T, unsigned int MAX_FENCES> TAGResourceManager::RingBuffer<T, MAX_FENCES>::~RingBuffer() {
	glUnmapNamedBuffer(buffer_id);
	deleteBuffer<GenericBuffer>(buffer_id);
}

template<class T, unsigned int MAX_FENCES> void TAGResourceManager::RingBuffer<T, MAX_FENCES>::updateBuffer(const std::vector<T>& data) {
	current_fence = (current_fence + 1) % MAX_FENCES;
	internal_offset = max_objs * current_fence * sizeof(T);

	if (fences[current_fence].sync) {
		while (glClientWaitSync(fences[current_fence].sync, 0, 0) == GL_TIMEOUT_EXPIRED) {
			continue;
		}
		glDeleteSync(fences[current_fence]);
		fences[current_fence].sync = nullptr;
	}

	current_objs = glm::min(max_objs, data.size());
	std::memcpy(buffer_ptr + max_objs * current_fence, data.data(), current_objs * sizeof(T));
}

template<class T, unsigned int MAX_FENCES> void TAGResourceManager::RingBuffer<T, MAX_FENCES>::resizeBuffer(const unsigned int& new_size) {
	const GLuint new_buffer = createBuffer<GenericBuffer>();
	const unsigned int new_total_size = new_size * MAX_FENCES * sizeof(T);
	glNamedBufferStorage(new_buffer, new_total_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	const T* new_buffer_ptr = (T*)glMapNamedBufferRange(new_buffer, 0, new_total_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

	current_objs = glm::min(new_size, current_objs);
	std::memcpy(new_buffer_ptr, buffer_ptr + current_fence * max_objs, current_objs * sizeof(T));

	updateBindings(new_buffer);

	glUnmapNamedBuffer(buffer_id);
	deleteBuffer<GenericBuffer>(buffer_id);

	max_objs = new_size;
	buffer_id = new_buffer;
	buffer_ptr = new_buffer_ptr;
	current_fence = 0;
	internal_offset = 0;
	fences = std::make_unique<GLsync[]>(MAX_FENCES);
}

template<class T> TAGResourceManager::OrphanBuffer<T>::OrphanBuffer(const unsigned int& max_objs) {
	this->max_objs = max_objs;
	fences = std::make_unique<GLsyncWrap[]>(1);
	buffer_id = createBuffer<GenericBuffer>();

	glNamedBufferData(buffer_id, max_objs * sizeof(T), nullptr, (GLenum)access);
}

template<class T> TAGResourceManager::OrphanBuffer<T>::~OrphanBuffer() {
	deleteBuffer<GenericBuffer>(buffer_id);
}

template<class T> void TAGResourceManager::OrphanBuffer<T>::updateBuffer(const std::vector<T>& data) {
	glNamedBufferData(buffer_id, max_objs * sizeof(T), nullptr, (GLenum)access);
	current_objs = glm::min(max_objs, data.size());
	glNamedBufferSubData(buffer_id, 0, current_objs * sizeof(T), data.data());
}

template<class T> void TAGResourceManager::OrphanBuffer<T>::resizeBuffer(const unsigned int& new_size) {
	const GLuint new_buffer_id = createBuffer<GenericBuffer>(buffer_id);
	glNamedBufferData(new_buffer_id, new_size * sizeof(T), nullptr, (GLenum)access);

	current_objs = glm::min(new_size, current_objs);
	glCopyNamedBufferSubData(
		buffer_id,
		new_buffer_id,
		0,
		0,
		current_objs * sizeof(T)
	);

	updateBindings(new_buffer_id);

	deleteBuffer<GenericBuffer>(buffer_id);

	buffer_id = new_buffer_id;
	max_objs = new_size;
}

template<class C, class G, unsigned int DIVISIONS> TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::ObjectBuffer(const unsigned int& max_objs, G(*converter)(const C&, const unsigned int&),  const BufferAccess& access) {
	this->converter = converter;
	buffer = (access != BufferAccess::STREAM ? std::make_unique<OrphanBuffer<G>>(max_objs * (DIVISIONS + 1)) : std::make_unique<RingBuffer<G, 3>>(max_objs * (DIVISIONS + 1));
}

template<class C, class G, unsigned int DIVISIONS> const std::vector<C>& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::getObjects() const {
	return objs;
}

template<class C, class G, unsigned int DIVISIONS> std::vector<C>& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::changeObjects() {
	objects_changed = true;
	return objs;
}

template<class C, class G, unsigned int DIVISIONS> bool TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::isObjectsChanged() const {
	return objects_changed;
}

template<class C, class G, unsigned int DIVISIONS> const std::unique_ptr<TAGResourceManager::BufferHandler<G>>& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::getBuffer() {
	return buffer;
}

template<class C, class G, unsigned int DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::updateBuffer() {
	std::vector<G> buffer_data;
	const unsigned int buffer_data_size = objs.size() * (DIVISIONS + 1);
	buffer_data.reserve(buffer_data_size);

	if (buffer_data_size > buffer->getMaxObjects()) {
		buffer->resizeBuffer(objs.size());
	}

	for (const C& obj : objs) {
		for (unsigned int i = 0; i < DIVISIONS + 1; i++) {
			buffer_data.push_back(converter(obj, i));
		}
	}

	buffer->updateBuffer(buffer_data);
	objects_changed = false;
}

template<class T> requires std::derived_from<T, TAGResourceManager::OpenGLHandle> void TAGResourceManager::deleteBuffer(const GLuint& ID) {
	const auto it = std::find_if(buffers.begin(), buffers.end(), [&ID](const BufferVariant& buf) 
		{ 
			if (std::holds_alternative<T>(buf)) {
				return (std::get<T>(buf).getID() == ID);
			}
			return false;
		}
	);
	if (it != buffers.end()) {
		buffers.erase(it);
	}
}

template<class T> requires std::derived_from<T, TAGResourceManager::OpenGLHandle> GLuint TAGResourceManager::createBuffer() {
	buffers.emplace_back(std::in_place_type<T>);
	return std::get<T>(buffers.back()).getID();
}

template<class T> requires std::derived_from<T, TAGResourceManager::OpenGLHandle> bool TAGResourceManager::isBuffer(const GLuint& ID) {
	const auto it = std::find_if(buffers.begin(), buffers.end(), [&ID](const BufferVariant& buf)
		{
			if (std::holds_alternative<T>(buf)) {
				return (std::get<T>(buf).getID() == ID);
			}
			return false;
		}
	);
	return (it != buffers.end());
}