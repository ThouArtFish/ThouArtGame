#pragma once

#include <ResourceManagerClass.hpp>

template<OpenGLObjectType::Concept T> TAGResourceManager::OpenGLHandle<T>::OpenGLHandle() : OpenGLHandleWrapper(createObjectID(), T::TYPE_ID) {}

template<OpenGLObjectType::Concept T> TAGResourceManager::OpenGLHandle<T>::~OpenGLHandle() {
	std::cout << "ID: " + std::to_string(ID) + " | Name: " + (std::string)T::NAME.substr() + "\n";
	if constexpr (std::same_as<T, OpenGLObjectType::FragmentShader> || std::same_as<T, OpenGLObjectType::VertexShader>) {
		glDeleteShader(this->ID);
	}
	else if constexpr (std::same_as<T, OpenGLObjectType::ShaderProgram>) {
		glDeleteProgram(this->ID);
	}
	else if constexpr (std::same_as<T, OpenGLObjectType::GenericBuffer>) {
		glDeleteBuffers(1, &this->ID);
	}
	else if constexpr (std::same_as<T, OpenGLObjectType::TextureBuffer>) {
		glDeleteTextures(1, &this->ID);
	}
	else { // OpenGLObjectType::VertexArrayObject
		glDeleteVertexArrays(1, &this->ID);
	}
}

template<OpenGLObjectType::Concept T> GLuint TAGResourceManager::OpenGLHandle<T>::createObjectID() {
	if constexpr (std::same_as<T, OpenGLObjectType::FragmentShader>) {
		return glCreateShader(GL_FRAGMENT_SHADER);
	} 
	else if constexpr (std::same_as<T, OpenGLObjectType::VertexShader>) {
		return glCreateShader(GL_VERTEX_SHADER);
	}
	else if constexpr (std::same_as<T, OpenGLObjectType::ShaderProgram>) {
		return glCreateProgram();
	}
	else if constexpr (std::same_as<T, OpenGLObjectType::GenericBuffer>) {
		GLuint buf_ID;
		glCreateBuffers(1, &buf_ID);
		return buf_ID;
	}
	else if constexpr (std::same_as<T, OpenGLObjectType::TextureBuffer>) {
		GLuint tex_ID;
		glGenTextures(1, &tex_ID);
		return tex_ID;
	}
	else { // OpenGLObjectType::VertexArrayObject
		GLuint vao_ID;
		glGenVertexArrays(1, &vao_ID);
		return vao_ID;
	}
}

template<OpenGLObjectType::Concept T> GLuint TAGResourceManager::createBuffer() {
	buffers.emplace_back(std::make_unique<OpenGLHandle<T>>());
	return buffers.back()->ID;
}

template<OpenGLObjectType::Concept T> void TAGResourceManager::deleteBuffer(const GLuint& ID) {
	const auto it = std::find_if(buffers.begin(), buffers.end(), [&ID](const std::unique_ptr<OpenGLHandleWrapper>& buf)
		{
			return (ID == buf->ID && T::TYPE_ID == buf->TYPE_ID);
		}
	);
	if (it != buffers.end()) {
		buffers.erase(it);
	}
}

template<OpenGLObjectType::Concept T> bool TAGResourceManager::isBuffer(const GLuint& ID) {
	const auto it = std::find_if(buffers.begin(), buffers.end(), [&ID](const std::unique_ptr<OpenGLHandleWrapper>& buf)
		{
			return (ID == buf->ID && T::TYPE_ID == buf->TYPE_ID);
		}
	);
	return (it != buffers.end());
}

template<class T> TAGResourceManager::BufferHandler<T>::BufferHandler(const bool& include_size, const GLuint& max_objs, const BufferAccess& access) : include_size(include_size ? (GLuint) glm::max(alignof(T), sizeof(GLuint)) : 0), max_objs(max_objs), access(access) {}

template<class T> GLuint TAGResourceManager::BufferHandler<T>::getMaxObjects() const {
	return max_objs;
}

template<class T> GLuint TAGResourceManager::BufferHandler<T>::getCurrentObjects() const {
	return current_objs;
}

template<class T> GLuint TAGResourceManager::BufferHandler<T>::getBufferID() const {
	return buffer_id;
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

template<class T, GLuint MAX_FENCES> TAGResourceManager::RingBuffer<T, MAX_FENCES>::RingBuffer(const GLuint& max_objs, const bool& include_size) : BufferHandler<T>(include_size, max_objs, BufferAccess::STREAM) {
	this->fences = std::make_unique<GLsyncWrap[]>(MAX_FENCES);

	const unsigned int total_size = (max_objs * sizeof(T) + this->include_size) * MAX_FENCES;
	this->buffer_id = createBuffer<OpenGLObjectType::GenericBuffer>();
	glNamedBufferStorage(this->buffer_id, total_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	this->buffer_ptr = (GLchar*) glMapNamedBufferRange(this->buffer_id, 0, total_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

template<class T, GLuint MAX_FENCES> TAGResourceManager::RingBuffer<T, MAX_FENCES>::~RingBuffer() {
	glUnmapNamedBuffer(this->buffer_id);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(this->buffer_id);
}

template<class T, GLuint MAX_FENCES> void TAGResourceManager::RingBuffer<T, MAX_FENCES>::updateBuffer(const std::vector<T>& data) {
	this->current_fence = (this->current_fence + 1) % MAX_FENCES;
	this->internal_offset = (this->max_objs * sizeof(T) + this->include_size) * this->current_fence;

	if (this->fences[this->current_fence].sync) {
		while (glClientWaitSync(this->fences[this->current_fence].sync, 0, 0) == GL_TIMEOUT_EXPIRED) {
			continue;
		}
		glDeleteSync(this->fences[this->current_fence].sync);
		this->fences[this->current_fence].sync = nullptr;
	}

	this->current_objs = glm::min(this->max_objs, (GLuint) data.size());
	if (this->include_size > 0) {
		(GLuint&)*(this->buffer_ptr + this->internal_offset) = this->current_objs;
	}
	std::memcpy(this->buffer_ptr + this->internal_offset + this->include_size, data.data(), this->current_objs * sizeof(T));
}

template<class T, GLuint MAX_FENCES> void TAGResourceManager::RingBuffer<T, MAX_FENCES>::resizeBuffer(const GLuint& new_size) {
	const GLuint new_buffer = createBuffer<OpenGLObjectType::GenericBuffer>();
	const unsigned int new_total_size = (new_size * sizeof(T) + this->include_size) * MAX_FENCES;
	glNamedBufferStorage(new_buffer, new_total_size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	GLchar* new_buffer_ptr = (GLchar*) glMapNamedBufferRange(new_buffer, 0, new_total_size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

	this->current_objs = glm::min(new_size, this->current_objs);
	this->max_objs = new_size;
	if (this->include_size > 0) {
		(GLuint&)*new_buffer_ptr = this->current_objs;
	}
	std::memcpy(new_buffer_ptr + this->include_size, this->buffer_ptr + this->internal_offset + this->include_size, this->current_objs * sizeof(T));

	this->updateBindings(new_buffer);

	glUnmapNamedBuffer(this->buffer_id);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(this->buffer_id);

	this->buffer_id = new_buffer;
	this->buffer_ptr = new_buffer_ptr;
	this->current_fence = 0;
	this->internal_offset = 0;
	this->fences = std::make_unique<GLsyncWrap[]>(MAX_FENCES);
}

template<class T> TAGResourceManager::OrphanBuffer<T>::OrphanBuffer(const GLuint& max_objs, const BufferAccess& access, const bool& include_size) : BufferHandler<T>(include_size, max_objs, access) {
	this->fences = std::make_unique<GLsyncWrap[]>(1);
	this->buffer_id = createBuffer<OpenGLObjectType::GenericBuffer>();

	glNamedBufferData(this->buffer_id, max_objs * sizeof(T) + this->include_size, nullptr, (GLenum)access);
}

template<class T> TAGResourceManager::OrphanBuffer<T>::~OrphanBuffer() {
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(this->buffer_id);
}

template<class T> void TAGResourceManager::OrphanBuffer<T>::updateBuffer(const std::vector<T>& data) {
	glNamedBufferData(this->buffer_id, this->max_objs * sizeof(T) + this->include_size, nullptr, (GLenum)this->access);
	this->current_objs = glm::min(this->max_objs, (GLuint) data.size());
	if (this->include_size > 0) {
		glNamedBufferSubData(this->buffer_id, 0, this->include_size, &this->current_objs);
	}
	glNamedBufferSubData(this->buffer_id, this->include_size, this->current_objs * sizeof(T), data.data());
}

template<class T> void TAGResourceManager::OrphanBuffer<T>::resizeBuffer(const GLuint& new_size) {
	const GLuint new_buffer_id = createBuffer<OpenGLObjectType::GenericBuffer>();
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

	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(this->buffer_id);

	this->buffer_id = new_buffer_id;
}

template<class C, class G, GLuint DIVISIONS> TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::ObjectBuffer(const GLuint& max_objs, const std::function<G(const C&, const GLuint&)>& converter, const BufferAccess& access, const bool& include_size) {
	this->converter = converter;
	if (access != BufferAccess::STREAM) {
		this->buffer = std::make_unique<OrphanBuffer<G>>(max_objs * (DIVISIONS + 1), access, include_size);
	}
	else {
		this->buffer = std::make_unique<RingBuffer<G, 3>>(max_objs * (DIVISIONS + 1), include_size);
	}
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

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::bindToShader(const GLuint& binding_index, const GLintptr& offset, const ShaderBufferType& buffer_option) {
	auto& vec = shader_binding_indices[(GLuint)buffer_option];
	auto vec_it = std::find_if(vec.begin(), vec.end(), [&binding_index](const BindingData& data) { return data.binding_index == binding_index; });
	if ((vec_it != vec.end() && (vec_it->buffer_id != buffer->buffer_id || vec_it->offset != offset)) || vec_it == vec.end()) {
		glBindBufferRange((GLenum)buffer_option, binding_index, buffer->buffer_id, buffer->internal_offset + offset, buffer->max_objs * sizeof(G));
		if (std::find(buffer->bound_buffers.begin(), buffer->bound_buffers.end(), buffer_option) == buffer->bound_buffers.end()) {
			buffer->bound_buffers.push_back(buffer_option);
		}
		if (vec_it == vec.end()) {
			vec.emplace_back(this, binding_index, buffer->buffer_id, offset);
		}
		else {
			vec.emplace(vec_it, this, binding_index, buffer->buffer_id, offset);
		}
	}
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::bindToVertexArrayObject(const GLuint& binding_index, const GLintptr& offset, const GLuint& vao) {
	auto map_it = vao_binding_indices.find(vao);
	if (map_it != vao_binding_indices.end()) {
		auto& vec = map_it->second;
		auto vec_it = std::find_if(vec.begin(), vec.end(), [&binding_index](const BindingData& data) { return data.binding_index == binding_index; });
		if ((vec_it != vec.end() && (vec_it->buffer_id != buffer->buffer_id || vec_it->offset != offset)) || vec_it == vec.end()) {
			glVertexArrayVertexBuffer(vao, binding_index, buffer->buffer_id, buffer->internal_offset + offset + buffer->include_size, sizeof(G));
			if (std::find(buffer->bound_vaos.begin(), buffer->bound_vaos.end(), vao) == buffer->bound_vaos.end()) {
				buffer->bound_vaos.push_back(vao);
			}
			if (vec_it == vec.end()) {
				vec.emplace_back(this, binding_index, buffer->buffer_id, offset);
			}
			else {
				vec.emplace(vec_it, this, binding_index, buffer->buffer_id, offset);
			}
		}
	}
}

template<class C, class G, GLuint DIVISIONS> auto TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::begin() const {
	return objs.begin();
}

template<class C, class G, GLuint DIVISIONS> auto TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::end() const {
	return objs.end();
}
