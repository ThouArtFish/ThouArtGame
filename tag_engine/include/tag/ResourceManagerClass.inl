#pragma once

#include <ResourceManagerClass.hpp>

template<OpenGLObjectType::Concept T> TAGResourceManager::OpenGLHandle<T>::OpenGLHandle() : OpenGLHandleWrapper(createObjectID(), T::TYPE_ID) {}

template<OpenGLObjectType::Concept T> TAGResourceManager::OpenGLHandle<T>::~OpenGLHandle() {
	if constexpr (isAnyOf<T, OpenGLObjectType::VertexShader, OpenGLObjectType::FragmentShader, OpenGLObjectType::GeometryShader, OpenGLObjectType::ComputeShader>) {
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
	else if constexpr (std::same_as<T, OpenGLObjectType::Framebuffer>) {
		glDeleteFramebuffers(1, &this->ID);
	}
	else if constexpr (std::same_as<T, OpenGLObjectType::Renderbuffer>) {
		glDeleteRenderbuffers(1, &this->ID);
	}
	else { // OpenGLObjectType::VertexArrayObject
		vao_binding_indices.erase(this->ID);
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
	else if constexpr (std::same_as<T, OpenGLObjectType::GeometryShader>) {
		return glCreateShader(GL_GEOMETRY_SHADER);
	}
	else if constexpr (std::same_as<T, OpenGLObjectType::ComputeShader>) {
		return glCreateShader(GL_COMPUTE_SHADER);
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
	else if constexpr (std::same_as<T, OpenGLObjectType::Framebuffer>) {
		GLuint buf_ID;
		glCreateFramebuffers(1, &buf_ID);
		return buf_ID;
	}
	else if constexpr (std::same_as<T, OpenGLObjectType::Renderbuffer>) {
		GLuint buf_ID;
		glCreateRenderbuffers(1, &buf_ID);
		return buf_ID;
	}
	else { // OpenGLObjectType::VertexArrayObject
		GLuint vao_ID;
		glCreateVertexArrays(1, &vao_ID);
		vao_binding_indices[vao_ID] = {};
		return vao_ID;
	}
}

template<OpenGLObjectType::Concept T> GLuint TAGResourceManager::createBuffer() {
	buffers.emplace_back(std::make_unique<OpenGLHandle<T>>());
	return buffers.back()->ID;
}

template<OpenGLObjectType::Concept T> void TAGResourceManager::deleteBuffer(GLuint ID) {
	const auto it = std::find_if(buffers.begin(), buffers.end(), [&ID](const std::unique_ptr<OpenGLHandleWrapper>& buf)
		{
			return (ID == buf->ID && T::TYPE_ID == buf->TYPE_ID);
		}
	);
	if (it != buffers.end()) buffers.erase(it);
}

template<OpenGLObjectType::Concept T> bool TAGResourceManager::isBuffer(GLuint ID) {
	const auto it = std::find_if(buffers.begin(), buffers.end(), [&ID](const std::unique_ptr<OpenGLHandleWrapper>& buf)
		{
			return (ID == buf->ID && T::TYPE_ID == buf->TYPE_ID);
		}
	);
	return (it != buffers.end());
}

template<class T> TAGResourceManager::BufferBase<T>::BufferBase(size_t max_objs, size_t max_fences, BufferAccess access, bool include_size) : include_size(include_size ? glm::max(alignof(T), sizeof(GLuint)) : 0), max_objs(max_objs), max_fences(glm::max(max_fences, (size_t)1)), access(access) {}

template<class T> TAGResourceManager::BufferBase<T>::~BufferBase() {
	auto& vaos = TAGResourceManager::vao_binding_indices;
	for (GLuint vao : bound_vaos) {
		auto vec_it = vaos.find(vao);
		if (vec_it != vaos.end()) {
			std::vector<BindingData>& vec = vec_it->second;
			for (size_t i = 0; i < vec.size(); i++) {

				if (vec[i].buffer_id == buffer_id) vec.erase(vec.begin() + i--);
			}
		}
	}

	auto& shader_buffers = TAGResourceManager::shader_binding_indices;
	for (ShaderBufferType buffer_type : bound_buffers) {
		auto vec_it = shader_buffers.find((GLuint)buffer_type);
		if (vec_it != shader_buffers.end()) {
			std::vector<BindingData>& vec = vec_it->second;
			for (size_t i = 0; i < vec.size(); i++) {

				if (vec[i].buffer_id == buffer_id) vec.erase(vec.begin() + i--);
			}
		}
	}
}

template<class T> size_t TAGResourceManager::BufferBase<T>::getCurrentObjects() const {
	return current_objs;
}

template<class T> size_t TAGResourceManager::BufferBase<T>::getCurrentFence() const {
	return current_fence;
}

template<class T> size_t TAGResourceManager::BufferBase<T>::getMaxObjects() const {
	return max_objs;
}

template<class T> GLintptr TAGResourceManager::BufferBase<T>::getInternalOffset() const {
	return internal_offset;
}

template<class T> GLuint TAGResourceManager::BufferBase<T>::getBufferID() const {
	return buffer_id;
}

template<class T> int TAGResourceManager::BufferBase<T>::bindToShader(GLuint binding_index, ShaderBufferType buffer_option, GLintptr offset) {
	offset = glm::min(offset, (GLintptr)(max_objs * sizeof(T) + include_size));

	glBindBufferRange((GLenum)buffer_option, binding_index, buffer_id, internal_offset + offset, max_objs * sizeof(T) + include_size);

	if (std::find(bound_buffers.begin(), bound_buffers.end(), buffer_option) == bound_buffers.end()) bound_buffers.push_back(buffer_option);

	auto& vec = shader_binding_indices.try_emplace((GLuint)buffer_option).first->second;

	auto vec_it = std::find_if(vec.begin(), vec.end(), [&binding_index](const BindingData& data) { return data.binding_index == binding_index; });
	if (vec_it == vec.end()) {
		const size_t vec_size = vec.size();
		vec.emplace_back((BufferBaseWrapper*)this, nullptr, binding_index, buffer_id, offset);
		return vec_size;
	}
	else {
		*vec_it = BindingData{ (BufferBaseWrapper*)this, nullptr, binding_index, buffer_id, offset };
		return std::distance(vec.begin(), vec_it);
	}
}

template<class T> int TAGResourceManager::BufferBase<T>::bindToVertexArrayObject(GLuint binding_index, GLuint vao, GLintptr offset) {
	offset = glm::min(offset, (GLintptr)(max_objs * sizeof(T) + include_size));

	auto map_it = vao_binding_indices.find(vao);
	if (map_it != vao_binding_indices.end()) {
		glVertexArrayVertexBuffer(vao, binding_index, buffer_id, internal_offset + offset, sizeof(T));

		if (std::find(bound_vaos.begin(), bound_vaos.end(), vao) == bound_vaos.end()) bound_vaos.push_back(vao);

		auto& vec = vao_binding_indices.at(vao);

		auto vec_it = std::find_if(vec.begin(), vec.end(), [binding_index](const BindingData& data) { return data.binding_index == binding_index; });
		if (vec_it == vec.end()) {
			const size_t vec_size = vec.size();
			vec.emplace_back((BufferBaseWrapper*)this, nullptr, binding_index, buffer_id, offset);
			return vec_size;
		}
		else {
			*vec_it = BindingData{ (BufferBaseWrapper*)this, nullptr, binding_index, buffer_id, offset };
			return std::distance(vec.begin(), vec_it);
		}
	}
	return -1;
}

template<class T> void TAGResourceManager::BufferBase<T>::setFence(size_t fence_index) {
	if (fence_index >= max_fences) return;

	if (fences[fence_index].sync) glDeleteSync(fences[fence_index].sync);

	fences[fence_index] = { .sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0) };
}

template<class T> void TAGResourceManager::BufferBase<T>::setFence() {
	setFence(current_fence);
}

template<class T> void TAGResourceManager::BufferBase<T>::waitFence(size_t fence_index) {
	if (fence_index >= max_fences) return;

	if (fences[fence_index].sync) {
		while (glClientWaitSync(fences[fence_index].sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ULL) == GL_TIMEOUT_EXPIRED) continue;
		glDeleteSync(fences[fence_index].sync);
		fences[fence_index].sync = nullptr;
	}
}

template<class T> void TAGResourceManager::BufferBase<T>::incrementFence() {
	current_fence = (current_fence + 1) % max_fences;
	internal_offset = (max_objs * sizeof(T) + include_size) * current_fence;

	waitFence(current_fence);
}

template<class T> void TAGResourceManager::BufferBase<T>::updateBindings(GLuint new_buffer_id) {
	const GLintptr region_size = max_objs * sizeof(T) + include_size;
	for (size_t i = 0; i < bound_vaos.size(); i++) {
		auto map_it = vao_binding_indices.find(bound_vaos[i]);
		if (map_it == vao_binding_indices.end() || map_it->second.empty()) {
			bound_vaos.erase(bound_vaos.begin() + i--);
			continue;
		}
		bool contains_old_id = false;
		for (BindingData& binding_data : map_it->second) {
			if (binding_data.buffer_id == buffer_id) {
				binding_data.offset = glm::min(region_size, binding_data.offset);
				glVertexArrayVertexBuffer(map_it->first, binding_data.binding_index, new_buffer_id, internal_offset + binding_data.offset, sizeof(T));
				binding_data.buffer_id = new_buffer_id;
				contains_old_id = true;
			}
		};
		if (!contains_old_id) bound_vaos.erase(bound_vaos.begin() + i--);
	}
	for (size_t i = 0; i < bound_buffers.size(); i++) {
		auto map_it = shader_binding_indices.find((GLuint) bound_buffers[i]);
		if (map_it == shader_binding_indices.end() || map_it->second.empty()) {
			bound_buffers.erase(bound_buffers.begin() + i--);
			continue;
		}
		bool contains_old_id = false;
		for (BindingData& binding_data : map_it->second) {
			if (binding_data.buffer_id == buffer_id) {
				binding_data.offset = glm::min(region_size, binding_data.offset);
				glBindBufferRange((GLenum)map_it->first, binding_data.binding_index, new_buffer_id, internal_offset + binding_data.offset, region_size);
				binding_data.buffer_id = new_buffer_id;
				contains_old_id = true;
			}
		};
		if (!contains_old_id) bound_buffers.erase(bound_buffers.begin() + i--);
	}
}

template<class T> TAGResourceManager::MappedBuffer<T>::MappedBuffer(size_t max_objs, size_t max_fences, BufferAccess access, const std::vector<MappedBufferAccess>& map_access, bool include_size) : BufferBase<T>(max_objs, max_fences, access, include_size) {
	this->fences = std::make_unique<GLsyncWrap[]>(max_fences);

	const size_t total_size = (max_objs * sizeof(T) + this->include_size) * glm::max((size_t)1, max_fences);
	this->buffer_id = createBuffer<OpenGLObjectType::GenericBuffer>();
	this->mapped_access_bitfield = 0;
	for (MappedBufferAccess field : map_access) this->mapped_access_bitfield |= (GLuint)field;
	glNamedBufferStorage(this->buffer_id, total_size, nullptr, this->mapped_access_bitfield);
	this->buffer_ptr = static_cast<GLchar*>(glMapNamedBufferRange(this->buffer_id, 0, total_size, this->mapped_access_bitfield));
}

template<class T> TAGResourceManager::MappedBuffer<T>::~MappedBuffer() {
	glUnmapNamedBuffer(this->buffer_id);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(this->buffer_id);
}

template<class T> void TAGResourceManager::MappedBuffer<T>::updateBuffer(const std::vector<T>& data) {
	this->incrementFence();

	this->current_objs = glm::min(this->max_objs, data.size());
	if (this->include_size > 0) std::memcpy(this->buffer_ptr + this->internal_offset, &this->current_objs, sizeof(GLuint));
	std::memcpy(this->buffer_ptr + this->internal_offset + this->include_size, (GLchar*)data.data(), this->current_objs * sizeof(T));

	this->setFence(this->current_fence);
	if (this->max_fences > 1) this->updateBindings(this->buffer_id);
}

template<class T> void TAGResourceManager::MappedBuffer<T>::resizeBuffer(size_t new_size) {
	this->waitFence(this->current_fence);

	const GLuint new_buffer = createBuffer<OpenGLObjectType::GenericBuffer>();
	const size_t new_total_size = (new_size * sizeof(T) + this->include_size) * this->max_fences;
	glNamedBufferStorage(new_buffer, new_total_size, nullptr, this->mapped_access_bitfield);
	GLchar* new_buffer_ptr = static_cast<GLchar*>(glMapNamedBufferRange(new_buffer, 0, new_total_size, this->mapped_access_bitfield));

	this->current_objs = glm::min(new_size, this->current_objs);
	if (this->include_size > 0) (GLuint&)*new_buffer_ptr = this->current_objs;
	std::memcpy(new_buffer_ptr + this->include_size, this->buffer_ptr + this->include_size + (this->max_objs * sizeof(T) + this->include_size) * this->current_fence, this->current_objs * sizeof(T));

	this->max_objs = new_size;
	this->current_fence = 0;
	this->internal_offset = 0;
	this->fences = std::make_unique<GLsyncWrap[]>(this->max_fences);

	this->updateBindings(new_buffer);

	glUnmapNamedBuffer(this->buffer_id);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(this->buffer_id);

	this->buffer_id = new_buffer;
	this->buffer_ptr = new_buffer_ptr;
	this->setFence(0);
}

template<class T> std::span<GLchar> TAGResourceManager::MappedBuffer<T>::bufferPtr(size_t fence_index) {
	if (fence_index >= this->max_fences) return nullptr;

	this->waitFence(fence_index);

	return { buffer_ptr + this->internal_offset, this->max_objs * sizeof(T) };
}

template<class T> TAGResourceManager::UnmappedBuffer<T>::UnmappedBuffer(size_t max_objs, size_t max_fences, BufferAccess access, bool include_size) : BufferBase<T>(max_objs, max_fences, access, include_size) {
	this->fences = std::make_unique<GLsyncWrap[]>(max_fences);
	this->buffer_id = createBuffer<OpenGLObjectType::GenericBuffer>();

	glNamedBufferData(this->buffer_id, (max_objs * sizeof(T) + this->include_size) * glm::max((size_t)1, max_fences), nullptr, (GLenum)access);
}

template<class T> TAGResourceManager::UnmappedBuffer<T>::~UnmappedBuffer() {
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(this->buffer_id);
}

template<class T> void TAGResourceManager::UnmappedBuffer<T>::updateBuffer(const std::vector<T>& data) {
	this->incrementFence();

	this->current_objs = glm::min((size_t)this->max_objs, data.size());

	if (this->include_size > 0) glNamedBufferSubData(this->buffer_id, this->internal_offset, sizeof(GLuint), &this->current_objs);
	glNamedBufferSubData(this->buffer_id, this->internal_offset + this->include_size, this->current_objs * sizeof(T), data.data());

	this->setFence(this->current_fence);
	if (this->max_fences > 1) this->updateBindings(this->buffer_id);
}

template<class T> void TAGResourceManager::UnmappedBuffer<T>::resizeBuffer(size_t new_size) {
	this->waitFence(this->current_fence);

	const GLuint new_buffer_id = createBuffer<OpenGLObjectType::GenericBuffer>();
	glNamedBufferData(new_buffer_id, (new_size * sizeof(T) + this->include_size) * this->max_fences, nullptr, (GLenum)this->access);

	this->current_objs = glm::min(new_size, (size_t)this->current_objs);
	glCopyNamedBufferSubData(
		this->buffer_id,
		new_buffer_id,
		(this->max_objs * sizeof(T) + this->include_size) * this->current_fence + this->include_size,
		this->include_size,
		this->current_objs * sizeof(T)
	);
	if (this->include_size > 0) glNamedBufferSubData(new_buffer_id, 0, sizeof(GLuint), &this->current_objs);

	this->max_objs = new_size;
	this->current_fence = 0;
	this->internal_offset = 0;
	this->fences = std::make_unique<GLsyncWrap[]>(this->max_fences);

	this->updateBindings(new_buffer_id);

	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(this->buffer_id);

	this->buffer_id = new_buffer_id;
	this->setFence(0);
}

template<class C, class G, GLuint DIVISIONS> TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::ObjectBuffer(size_t max_objs, const std::function<std::array<G, DIVISIONS + 1>(const C&)>& converter, BufferAccess access, bool include_size) {
	this->converter = converter;
	if (access != BufferAccess::STREAM) {
		this->buffer = std::make_unique<UnmappedBuffer<G>>(max_objs * (DIVISIONS + 1), 1, access, include_size);
	}
	else {
		this->buffer = std::make_unique<MappedBuffer<G>>(max_objs * (DIVISIONS + 1), 3, access, std::vector<MappedBufferAccess>{ MappedBufferAccess::CLIENT_WRITE, MappedBufferAccess::COHERENT, MappedBufferAccess::PERSISTENT }, include_size);
	}
}

template<class C, class G, GLuint DIVISIONS> const std::vector<C>& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::getAllObjects() const {
	return objs;
}

template<class C, class G, GLuint DIVISIONS> const C& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::getObject(size_t index) const {
	return objs.at(index);
}

template<class C, class G, GLuint DIVISIONS> const C& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::peekObject() const {
	return objs.back();
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::pushObject(const C& obj) {
	objs.push_back(obj);
	this->objects_changed = true;
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::setObject(const C& obj, size_t index) {
	objs.at(index) = obj;
	this->objects_changed = true;
}

template<class C, class G, GLuint DIVISIONS> template<class T> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::setObjectMember(const T& value, size_t offset, size_t index) {
	(T&)*((GLchar*)&objs.at(index) + offset) = value;
	this->objects_changed = true;
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::insertObject(const C& obj, size_t index) {
	objs.insert(objs.begin() + index, obj);
	this->objects_changed = true;
}

template<class C, class G, GLuint DIVISIONS> C TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::popObject() {
	C obj = objs.back();
	objs.pop_back();
	this->objects_changed = true;
	return obj;
}

template<class C, class G, GLuint DIVISIONS> C TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::removeObject(size_t index) {
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

template<class C, class G, GLuint DIVISIONS> const std::unique_ptr<TAGResourceManager::BufferBase<G>>& TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::getBuffer() const {
	return buffer;
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::bindToShader(GLuint binding_index, ShaderBufferType buffer_option, GLintptr offset) {
	int index = buffer->bindToShader(binding_index, buffer_option, offset);

	shader_binding_indices.at((GLuint)buffer_option).at(index).object_ptr = (ObjectBufferWrapper*)this;
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::bindToVertexArrayObject(GLuint binding_index, GLuint vao, GLintptr offset) {
	int index = buffer->bindToVertexArrayObject(binding_index, vao, buffer->include_size + offset);

	if (index) vao_binding_indices.at(vao).at(index).object_ptr = (ObjectBufferWrapper*)this;
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::updateBuffer() {
	std::vector<G> buffer_data;
	const size_t buffer_data_size = objs.size() * (DIVISIONS + 1);
	buffer_data.reserve(buffer_data_size);

	if (buffer_data_size > buffer->getMaxObjects()) buffer->resizeBuffer(buffer_data_size * 2u);

	for (const C& obj : objs) {
		const std::array<G, DIVISIONS + 1> data = converter(obj);
		buffer_data.insert(buffer_data.end(), data.begin(), data.end());
	}

	buffer->updateBuffer(buffer_data);
	this->objects_changed = false;
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::setFence() {
	buffer->setFence(buffer->getCurrentFence());
}

template<class C, class G, GLuint DIVISIONS> void TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::waitFence() {
	buffer->waitFence(buffer->getCurrentFence());
}

template<class C, class G, GLuint DIVISIONS> auto TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::begin() const {
	return objs.begin();
}

template<class C, class G, GLuint DIVISIONS> auto TAGResourceManager::ObjectBuffer<C, G, DIVISIONS>::end() const {
	return objs.end();
}
