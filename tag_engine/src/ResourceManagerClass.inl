#pragma once

#include <ResourceManagerClass.hpp>

template<class T> unsigned int TAGResourceManager::BufferHandler<T>::getMaxObjects() const {
	return max_objs;
}

template<class T> unsigned int TAGResourceManager::BufferHandler<T>::getCurrentObjects() const {
	return current_objs;
}

template<class T> BufferAccess TAGResourceManager::BufferHandler<T>::getAccessLevel() const {
	return access;
}

template<class T, unsigned int MAX_FENCES> TAGResourceManager::RingBuffer<T, MAX_FENCES>::RingBuffer(const unsigned int& max_objs) {
	access = BufferAccess::STREAM;
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
	
	current_objs = glm::min(data.size(), max_objs);
	std::memcpy(buffer_ptr + max_objs * current_fence, data.data(), current_objs * sizeof(T));
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
	const T* new_buffer_ptr = (T*)glMapNamedBufferRange(new_buffer, 0, new_total_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

	const unsigned int new_current_objs = glm::min(new_size, current_objs);
	std::memcpy(new_buffer_ptr, buffer_ptr + current_fence * max_objs, new_current_objs * sizeof(T));

	glUnmapNamedBuffer(buffer_id);
	deleteBuffer<GenericBuffer>(buffer_id);

	max_objs = new_size;
	current_objs = new_current_objs;
	buffer_id = new_buffer;
	buffer_ptr = new_buffer_ptr;
	current_fence = 0;
	fences = {};
}

template<class T, unsigned int MAX_FENCES> void TAGResourceManager::RingBuffer<T, MAX_FENCES>::setFence() {
	fences[current_fence] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

template<class T> TAGResourceManager::OrphanBuffer<T>::OrphanBuffer(const unsigned int& max_objs, const BufferAccess& access) {
	this->max_objs = max_objs;
	this->access = access;
	buffer_id = createBuffer<GenericBuffer>();

	glNamedBufferStorage(buffer_id, max_objs * sizeof(T), nullptr, (GLenum)access);
}

template<class T> TAGResourceManager::OrphanBuffer<T>::~OrphanBuffer() {
	deleteBuffer<GenericBuffer>(buffer_id);
}

template<class T> void TAGResourceManager::OrphanBuffer<T>::updateBuffer(const std::vector<T>& data) {
	glNamedBufferData(buffer_id, max_objs * sizeof(T), nullptr, (GLenum)access);
	current_objs = glm::min(data.size(), max_objs);
	glNamedBufferSubData(buffer_id, 0, sizeof(T) * current_objs, data.data());
}

template<class T> void TAGResourceManager::OrphanBuffer<T>::bindBuffer(const GLuint& binding_index, const GLuint& vao) const {
	if (vao > 0) {
		glVertexArrayVertexBuffer(vao, binding_index, buffer_id, 0, sizeof(T));
	}
	else {
		glBindVertexBuffer(binding_index, buffer_id, 0, sizeof(T));
	}
};

template<class T> void TAGResourceManager::OrphanBuffer<T>::resizeBuffer(const unsigned int& new_size) {
	const GLuint new_buffer_id = createBuffer<GenericBuffer>(buffer_id);
	glNamedBufferData(new_buffer_id, new_size * sizeof(T), nullptr, (GLenum)access);

	const unsigned int new_current_objs = glm::min(new_size, current_objs);
	glCopyNamedBufferSubData(
		buffer_id,
		new_buffer_id,
		0,
		0,
		new_current_objs * sizeof(T)
	);

	deleteBuffer<GenericBuffer>(buffer_id);
	buffer_id = new_buffer_id;
	max_objs = new_size;
	current_objs = new_current_objs;
}

template<class C, class G> TAGResourceManager::ObjectBuffer<C, G>::ObjectBuffer(const unsigned int& max_objs, G(*converter)(const C&),  const BufferAccess& access) {
	this->converter = converter;
	buffer = (access != BufferAccess::STREAM ? std::make_unique<OrphanBuffer<G>>(max_objs, access) : std::make_unique<RingBuffer<G, 3>>(max_objs));
}

template<class C, class G> const std::vector<C>& TAGResourceManager::ObjectBuffer<C, G>::getObjects() const {
	return objs;
}

template<class C, class G> std::vector<C>& TAGResourceManager::ObjectBuffer<C, G>::changeObjects() {
	objects_changed = true;
	return objs;
}

template<class C, class G> bool TAGResourceManager::ObjectBuffer<C, G>::isObjectsChanged() const {
	return objects_changed;
}

template<class C, class G> const TAGResourceManager::BufferHandler<G>& TAGResourceManager::ObjectBuffer<C, G>::getBuffer() const {
	return *(buffer.get());
}

template<class C, class G> void TAGResourceManager::ObjectBuffer<C, G>::updateBuffer() {
	std::vector<G> buffer_data;
	buffer_data.reserve(objs.size());

	for (const C& obj : objs) {
		buffer_data.push_back(converter(obj));
	}

	buffer->updateBuffer(buffer_data);
}

template<class C, class G> void TAGResourceManager::ObjectBuffer<C, G>::resizeBuffer(const unsigned int& new_size) {
	buffer->resizeBuffer(new_size);
}

template<class C, class G> void TAGResourceManager::ObjectBuffer<C, G>::changeAccess(const BufferAccess& new_access) {
	if (buffer->getAccessLevel() == new_access) {
		return;
	}

	const BufferHandler<G>* old_buffer = buffer.release();
	buffer = (new_access != BufferAccess::STREAM ? std::make_unique<OrphanBuffer<G>>(old_buffer->getMaxObjects(), new_access) : std::make_unique<RingBuffer<G, 3>>(old_buffer->getMaxObjects()));
	updateBuffer();
	delete old_buffer;
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