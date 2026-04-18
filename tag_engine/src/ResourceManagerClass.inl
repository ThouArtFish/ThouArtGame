#pragma once

#include <ResourceManagerClass.hpp>

template<class T, unsigned int MAX_FENCES> TAGResourceManager::RingBuffer<T, MAX_FENCES>::RingBuffer(const unsigned int& max_objs) {
	this->max_objs = max_objs;
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

	if (fences[current_fence]) {
		while (glClientWaitSync(fences[current_fence], 0, 0) == GL_TIMEOUT_EXPIRED) {
			continue;
		}
		glDeleteSync(fences[current_fence]);
		fences[current_fence] = nullptr;
	}
	
	current_objs = (unsigned int)data.size();
	std::memcpy(buffer_ptr + max_objs * current_fence, data.data(), glm::min(max_objs, current_objs) * sizeof(T));
}

template<class T, unsigned int MAX_FENCES> void TAGResourceManager::RingBuffer<T, MAX_FENCES>::bindBuffer(const GLuint& binding_index, const GLuint& vao) const {
	if (vao > 0) {
		glVertexArrayVertexBuffer(vao, binding_index, buffer_id, (GLintptr)(current_fence * max_objs * sizeof(T)), sizeof(T));
	}
	else {
		glBindVertexBuffer(binding_index, buffer_id, (GLintptr)(current_fence * max_objs * sizeof(T)), sizeof(T));
	}
}

template<class T, unsigned int MAX_FENCES> void TAGResourceManager::RingBuffer<T, MAX_FENCES>::resizeBuffer(const unsigned int& new_size) {
	const GLuint new_buffer = createBuffer<GenericBuffer>();
	const unsigned int new_total_size = new_size * sizeof(T) * MAX_FENCES;
	glNamedBufferStorage(new_buffer, new_total_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	T* new_buffer_ptr = (T*)glMapNamedBufferRange(new_buffer, 0, new_total_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

	std::memcpy(new_buffer_ptr, buffer_ptr + current_fence * max_objs * sizeof(T), current_objs * sizeof(T));

	glUnmapNamedBuffer(buffer_id);
	deleteBuffer<GenericBuffer>(buffer_id);

	max_objs = new_size;
	buffer_id = new_buffer;
	buffer_ptr = new_buffer_ptr;
	current_fence = 0;
	fences = {};
}

template<class T, unsigned int MAX_FENCES> void TAGResourceManager::RingBuffer<T, MAX_FENCES>::setFence() {
	fences[current_fence] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

template<class T, unsigned int MAX_FENCES> const unsigned int& TAGResourceManager::RingBuffer<T, MAX_FENCES>::getMaxObjects() const {
	return max_objs;
}

template<BufferType T> void TAGResourceManager::deleteBuffer(const GLuint& ID) {
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

template<BufferType T> GLuint TAGResourceManager::createBuffer() {
	buffers.emplace_back(std::in_place_type<T>);
	return std::get<T>(buffers.back()).getID();
}

template<BufferType T> bool TAGResourceManager::isBuffer(const GLuint& ID) {
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