#pragma once

#include <ResourceManagerClass.hpp>

template<class T> TAGResourceManager::BufferHandler<T>::BufferHandler(const bool& include_size, const BufferAccess& access) : include_size(include_size ? glm::max(alignof(T), sizeof(GLuint)) : 0), access(access) {}

template<class T> GLuint TAGResourceManager::BufferHandler<T>::getMaxObjects() const {
	return max_objs;
}

template<class T> GLuint TAGResourceManager::BufferHandler<T>::getCurrentObjects() const {
	return current_objs;
}

template<class T> GLuint TAGResourceManager::BufferHandler<T>::getBufferID() const {
	return buffer_id;
}

template<class T> void TAGResourceManager::BufferHandler<T>::bindToShader(const GLuint& binding_index, const GLintptr& offset, const ShaderBufferType& buffer_option) {
	auto& vec = shader_binding_indices[(GLuint) buffer_option];
	auto vec_it = std::find_if(vec.begin(), vec.end(), [&binding_index](const BindingData& data) { return data.binding_index == binding_index; });
	if ((vec_it != vec.end() && (vec_it->buffer_id != buffer_id || vec_it->offset != offset)) || vec_it == vec.end()) {
		glBindBufferRange((GLenum)buffer_option, binding_index, buffer_id, internal_offset + offset, max_objs * sizeof(T));
		if (std::find(bound_buffers.begin(), bound_buffers.end(), buffer_option) == bound_buffers.end()) {
			bound_buffers.push_back(buffer_option);
		}
		if (vec_it == vec.end()) {
			vec.emplace_back(this, binding_index, buffer_id, offset);
		}
		else {
			vec.emplace(vec_it, this, binding_index, buffer_id, offset);
		}
	}
}

template<class T> void TAGResourceManager::BufferHandler<T>::bindToVertexArrayObject(const GLuint& binding_index, const GLintptr& offset, const GLuint& vao) {
	auto map_it = vao_binding_indices.find(vao);
	if (map_it != vao_binding_indices.end()) {
		auto& vec = map_it->second;
		auto vec_it = std::find_if(vec.begin(), vec.end(), [&binding_index](const BindingData& data) { return data.binding_index == binding_index; });
		if ((vec_it != vec.end() && (vec_it->buffer_id != buffer_id || vec_it->offset != offset)) || vec_it == vec.end()) {
			glVertexArrayVertexBuffer(vao, binding_index, buffer_id, internal_offset + offset + include_size, sizeof(T));
			if (std::find(bound_vaos.begin(), bound_vaos.end(), vao) == bound_vaos.end()) {
				bound_vaos.push_back(vao);
			}
			if (vec_it == vec.end()) {
				vec.emplace_back(this, binding_index, buffer_id, offset);
			}
			else {
				vec.emplace(vec_it, this, binding_index, buffer_id, offset);
			}
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
			if (binding_data.buffer_id == buffer_id && binding_data.ptr) {
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
		auto map_it = shader_binding_indices.find((GLuint) bound_buffers[i]);
		if (map_it == shader_binding_indices.end() || map_it->second.empty()) {
			bound_buffers.erase(bound_buffers.begin() + i--);
			continue;
		}
		bool contains_old_id = false;
		for (BindingData& binding_data : map_it->second) {
			if (binding_data.buffer_id == buffer_id) {
				glBindBufferRange((GLenum)map_it->first, binding_data.binding_index, new_buffer_id, binding_data.offset, max_objs * sizeof(T));
				binding_data.buffer_id = new_buffer_id;
				contains_old_id = true;
			}
		};
		if (!contains_old_id) {
			bound_buffers.erase(bound_buffers.begin() + i--);
		}
	}
}

template<class T, GLuint MAX_FENCES> TAGResourceManager::RingBuffer<T, MAX_FENCES>::RingBuffer(const GLuint& max_objs, const bool& include_size) : BufferHandler(include_size, BufferAccess::STREAM) {
	this->max_objs = max_objs;
	this->fences = std::make_unique<GLsyncWrap[]>(MAX_FENCES);

	const unsigned int total_size = (max_objs * sizeof(T) + this->include_size) * MAX_FENCES;
	this->buffer_id = createBuffer(OpenGLObjectType::GENERIC_BUFFER);
	glNamedBufferStorage(this->buffer_id, total_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	this->buffer_ptr = glMapNamedBufferRange(this->buffer_id, 0, total_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

template<class T, GLuint MAX_FENCES> TAGResourceManager::RingBuffer<T, MAX_FENCES>::~RingBuffer() {
	glUnmapNamedBuffer(this->buffer_id);
	deleteBuffer(this->buffer_id, TAGResourceManager::OpenGLObjectType::GENERIC_BUFFER);
}

template<class T, GLuint MAX_FENCES> void TAGResourceManager::RingBuffer<T, MAX_FENCES>::updateBuffer(const std::vector<T>& data) {
	this->current_fence = (this->current_fence + 1) % MAX_FENCES;
	this->internal_offset = (this->max_objs * sizeof(T) + this->include_size) * this->current_fence;

	if (this->fences[this->current_fence].sync) {
		while (glClientWaitSync(this->fences[this->current_fence].sync, 0, 0) == GL_TIMEOUT_EXPIRED) {
			continue;
		}
		glDeleteSync(this->fences[this->current_fence]);
		this->fences[this->current_fence].sync = nullptr;
	}

	this->current_objs = glm::min(this->max_objs, data.size());
	if (this->include_size > 0) {
		(GLuint&)*(this->buffer_ptr + this->internal_offset) = this->current_objs;
	}
	std::memcpy(this->buffer_ptr + this->internal_offset + this->include_size, data.data(), this->current_objs * sizeof(T));
}

template<class T, GLuint MAX_FENCES> void TAGResourceManager::RingBuffer<T, MAX_FENCES>::resizeBuffer(const GLuint& new_size) {
	const GLuint new_buffer = createBuffer(OpenGLObjectType::GENERIC_BUFFER);
	const unsigned int new_total_size = (new_size * sizeof(T) + this->include_size) * MAX_FENCES;
	glNamedBufferStorage(new_buffer, new_total_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	const GLchar* new_buffer_ptr = glMapNamedBufferRange(new_buffer, 0, new_total_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

	this->current_objs = glm::min(new_size, this->current_objs);
	this->max_objs = new_size;
	if (this->include_size > 0) {
		(GLuint&)*new_buffer_ptr = this->current_objs;
	}
	std::memcpy(new_buffer_ptr + this->include_size, this->buffer_ptr + this->internal_offset + this->include_size, this->current_objs * sizeof(T));

	this->updateBindings(new_buffer);

	glUnmapNamedBuffer(this->buffer_id);
	deleteBuffer(this->buffer_id, TAGResourceManager::OpenGLObjectType::GENERIC_BUFFER);

	this->buffer_id = new_buffer;
	this->buffer_ptr = new_buffer_ptr;
	this->current_fence = 0;
	this->internal_offset = 0;
	this->fences = std::make_unique<GLsync[]>(MAX_FENCES);
}

template<class T> TAGResourceManager::OrphanBuffer<T>::OrphanBuffer(const GLuint& max_objs, const BufferAccess& access, const bool& include_size) : BufferHandler(include_size, access) {
	this->max_objs = max_objs;
	this->fences = std::make_unique<GLsyncWrap[]>(1);
	this->buffer_id = createBuffer(OpenGLObjectType::GENERIC_BUFFER);

	glNamedBufferData(this->buffer_id, max_objs * sizeof(T) + this->include_size, nullptr, (GLenum)access);
}

template<class T> TAGResourceManager::OrphanBuffer<T>::~OrphanBuffer() {
	deleteBuffer(this->buffer_id, TAGResourceManager::OpenGLObjectType::GENERIC_BUFFER);
}

template<class T> void TAGResourceManager::OrphanBuffer<T>::updateBuffer(const std::vector<T>& data) {
	glNamedBufferData(this->buffer_id, this->max_objs * sizeof(T) + this->include_size, nullptr, (GLenum)this->access);
	this->current_objs = glm::min(this->max_objs, data.size());
	if (this->include_size > 0) {
		glNamedBufferSubData(this->buffer_id, 0, this->include_size, &this->current_objs);
	}
	glNamedBufferSubData(this->buffer_id, this->include_size, this->current_objs * sizeof(T), data.data());
}

template<class T> void TAGResourceManager::OrphanBuffer<T>::resizeBuffer(const GLuint& new_size) {
	const GLuint new_buffer_id = createBuffer(OpenGLObjectType::GENERIC_BUFFER);
	glNamedBufferData(new_buffer_id, new_size * sizeof(T) + this->include_size, nullptr, (GLenum)this->access);

	this->max_objs = new_size;
	this->current_objs = glm::min(new_size, this->current_objs);
	if (this->include_size > 0) {
		glNamedBufferSubData(new_buffer_id, 0, this->include_size, &this->current_objs);
	}
	glCopyNamedBufferSubData(
		this->buffer_id,
		new_buffer_id,
		this->include_size,
		this->include_size,
		this->current_objs * sizeof(T)
	);

	this->updateBindings(new_buffer_id);

	deleteBuffer(this->buffer_id, TAGResourceManager::OpenGLObjectType::GENERIC_BUFFER);

	this->buffer_id = new_buffer_id;
}

bool TAGResourceManager::ObjectBufferWrapper::isObjectsChanged() const {
	return objects_changed;
}

template<class C, class G, GLuint DIVISIONS> TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::ObjectBuffer(const GLuint& max_objs, const std::function<G(const C&, const GLuint&)>& converter, const BufferAccess& access, const bool& include_size) {
	this->converter = converter;
	buffer = (access != BufferAccess::STREAM ? std::make_unique<OrphanBuffer<G>>(max_objs * (DIVISIONS + 1), access, include_size) : std::make_unique<RingBuffer<G, 3>>(max_objs * (DIVISIONS + 1), include_size));
}

template<class C, class G, GLuint DIVISIONS> TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::~ObjectBuffer() {
	auto& vaos = TAGResourceManager::vao_binding_indices;
	for (const GLuint& vao : buffer->bound_vaos) {
		auto vec_it = vaos.find(vao);
		if (vec_it != vaos.end()) {
			std::vector<BindingData>& vec = vec_it->second;
			for (BindingData& data : vec) {
				if (data.buffer_id == buffer->buffer_id) {
					data.ptr = nullptr;
				}
			}
		}
	}

	auto& shader_buffers = TAGResourceManager::shader_binding_indices;
	for (const ShaderBufferType& buffer_type : buffer->bound_buffers) {
		auto vec_it = shader_buffers.find((GLuint) buffer_type);
		if (vec_it != shader_buffers.end()) {
			std::vector<BindingData>& vec = vec_it->second;
			for (int i = 0; i < vec.size(); i++) {
				if (vec[i].buffer_id == buffer->buffer_id) {
					vec.erase(vec.begin() + i--);
				}
			}
		}
	}
}

template<class C, class G, GLuint DIVISIONS> const std::vector<C>& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::getAllObjects() const {
	return objs;
}

template<class C, class G, GLuint DIVISIONS> const C& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::getObject(const GLuint& index) const {
	return objs.at(index);
}

template<class C, class G, GLuint DIVISIONS> const C& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::peekObject() const {
	return objs.back();
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::pushObject(const C& obj) {
	objs.push_back(obj);
	this->objects_changed = true;
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::setObject(const C& obj, const GLuint& index) {
	objs.at(index) = obj;
	this->objects_changed = true;
}

template<class C, class G, GLuint DIVISIONS> template<class T> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::setObjectMember(const T& value, const GLuint& offset, const GLuint& index) {
	(T&)(*(&objs.at(index) + offset)) = value;
	this->objects_changed = true;
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::insertObject(const C& obj, const GLuint& index) {
	objs.insert(objs.begin() + index, obj);
	this->objects_changed = true;
}

template<class C, class G, GLuint DIVISIONS> C TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::popObject() {
	C obj = objs.back();
	objs.pop_back();
	this->objects_changed = true;
	return obj;
}

template<class C, class G, GLuint DIVISIONS> C TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::removeObject(const GLuint& index) {
	C obj = objs.at(index);
	objs.erase(objs.begin() + index);
	this->objects_changed = true;
	return obj;
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::clearObjects() {
	objs.clear();
	this->objects_changed = true;
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::setAllObjects(const std::vector<C>& objs) {
	this->objs = objs;
	this->objects_changed = true;
}

template<class C, class G, GLuint DIVISIONS> std::vector<C>& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::changeObjects() {
	this->objects_changed = true;
	return objs;
}

template<class C, class G, GLuint DIVISIONS> const std::unique_ptr<TAGResourceManager::BufferHandler<G>>& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::getBuffer() {
	return buffer;
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::updateBuffer() {
	std::vector<G> buffer_data;
	const GLuint buffer_data_size = (GLuint) objs.size() * (DIVISIONS + 1);
	buffer_data.reserve(buffer_data_size);

	if (buffer_data_size > buffer->getMaxObjects()) buffer->resizeBuffer((GLuint)((GLfloat)buffer_data_size * 1.2f));

	for (const C& obj : objs) {
		for (GLuint i = 0; i < DIVISIONS + 1; i++) {
			buffer_data.push_back(converter(obj, i));
		}
	}

	buffer->updateBuffer(buffer_data);
	this->objects_changed = false;
}

template<class C, class G, GLuint DIVISIONS> auto TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::begin() const {
	return objs.begin();
}

template<class C, class G, GLuint DIVISIONS> auto TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::end() const {
	return objs.end();
}
