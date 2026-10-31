#pragma once

#include <array>
#include <span>
#include <iostream>
#include <string>
#include <map>
#include <concepts>
#include <variant>
#include <vector>
#include <algorithm>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <UtilClass.hpp>

namespace OpenGLObjectType {
	struct TagStruct {};
	struct VertexArrayObject : TagStruct { constexpr static GLuint TYPE_ID = 0; constexpr inline static std::string_view NAME = "VAO"; };
	struct ShaderProgram : TagStruct { constexpr static GLuint TYPE_ID = 1; constexpr inline static std::string_view NAME = "ShaderProgram"; };
	struct VertexShader : TagStruct { constexpr static GLuint TYPE_ID = 2; constexpr inline static std::string_view NAME = "VertexShader"; };
	struct FragmentShader : TagStruct { constexpr static GLuint TYPE_ID = 3; constexpr inline static std::string_view NAME = "FragmentShader"; };
	struct GeometryShader : TagStruct { constexpr static GLuint TYPE_ID = 8; constexpr inline static std::string_view NAME = "GeometryShader"; };
	struct ComputeShader : TagStruct { constexpr static GLuint TYPE_ID = 9; constexpr inline static std::string_view NAME = "ComputeShader"; };
	struct TextureBuffer : TagStruct { constexpr static GLuint TYPE_ID = 4; constexpr inline static std::string_view NAME = "TextureBuffer"; };
	struct GenericBuffer : TagStruct { constexpr static GLuint TYPE_ID = 5; constexpr inline static std::string_view NAME = "GenericBuffer"; };
	struct Framebuffer : TagStruct { constexpr static GLuint TYPE_ID = 6; constexpr inline static std::string_view NAME = "Framebuffer"; };
	struct Renderbuffer : TagStruct { constexpr static GLuint TYPE_ID = 7; constexpr inline static std::string_view NAME = "Renderbuffer"; };

	template<class T> concept Concept = !std::same_as<T, TagStruct> && std::derived_from<T, TagStruct>;
};

/**
* Manages OpenGL buffer objects
*/
class TAGResourceManager {
	template<class C, class G, GLuint DIVISIONS> friend class ObjectBuffer;
	friend class OpenGLHandle;
public:
	/**
	* Structs for storing OpenGL buffer IDs
	*/
	struct OpenGLHandleWrapper {
		virtual ~OpenGLHandleWrapper() = default;

		OpenGLHandleWrapper(GLuint ID, GLuint TYPE_ID);
		const GLuint ID;
		const GLuint TYPE_ID;
	};

	template<OpenGLObjectType::Concept T> class OpenGLHandle : public OpenGLHandleWrapper {
	public:
		OpenGLHandle();
		~OpenGLHandle();
	private:
		static GLuint createObjectID();
	};

	/**
	* Buffer access levels
	*/
	enum class BufferAccess : GLuint {
		STATIC = GL_STATIC_DRAW,
		DYNAMIC = GL_DYNAMIC_DRAW,
		STREAM = GL_STREAM_DRAW
	};

	/**
	* Mapped buffer setup
	*/
	enum class MappedBufferAccess : GLuint {
		CLIENT_READ = GL_MAP_READ_BIT,
		CLIENT_WRITE = GL_MAP_WRITE_BIT,
		PERSISTENT =  GL_MAP_PERSISTENT_BIT,
		COHERENT = GL_MAP_COHERENT_BIT
	};

	/**
	* Type of buffer bound to shader
	*/
	enum class ShaderBufferType : GLuint {
		SHADER_STORAGE = GL_SHADER_STORAGE_BUFFER,
		ATOMIC_COUNTER = GL_ATOMIC_COUNTER_BUFFER,
		UNIFORM = GL_UNIFORM_BUFFER,
		TRANSFORM_FEEDBACK = GL_TRANSFORM_FEEDBACK_BUFFER
	};
	static constexpr inline std::array<ShaderBufferType, 4> buffer_types = { 
		ShaderBufferType::SHADER_STORAGE, 
		ShaderBufferType::ATOMIC_COUNTER, 
		ShaderBufferType::UNIFORM, 
		ShaderBufferType::TRANSFORM_FEEDBACK 
	};
	enum class ShaderBufferInterfaceType : GLuint {
		SHADER_STORAGE = GL_SHADER_STORAGE_BLOCK,
		ATOMIC_COUNTER = GL_ATOMIC_COUNTER_BUFFER,
		UNIFORM = GL_UNIFORM_BLOCK,
		TRANSFORM_FEEDBACK = GL_TRANSFORM_FEEDBACK_BUFFER
	};
	static constexpr inline std::array<ShaderBufferInterfaceType, 2> buffer_interface_types = { 
		ShaderBufferInterfaceType::SHADER_STORAGE, 
		ShaderBufferInterfaceType::UNIFORM
	};

	/**
	* The base path from the executable to loadable assets
	*/
	static inline std::string asset_path = "assets/";

	/**
	* Default constructible wrapper for GLsync objects
	*/
	struct GLsyncWrap {
		GLsync sync = nullptr;
	};

	/**
	* Templateless struct that allows tracking of BufferBase instances
	*/
	class BufferBaseWrapper {
	public:
		virtual void setFence() = 0;
	};

	/**
	* Interface for implementing buffer handling structs
	*/
	template<class T> class BufferBase : public BufferBaseWrapper {
		template<class C, class G, GLuint DIVISIONS> friend class ObjectBuffer;
	public:
		~BufferBase();

		const BufferAccess access;
		const size_t include_size, max_fences;

		size_t getCurrentObjects() const;
		size_t getCurrentFence() const;
		size_t getMaxObjects() const;
		GLintptr getInternalOffset() const;
		GLuint getBufferID() const;

		virtual void updateBuffer(const std::vector<T>& data) = 0;
		virtual void resizeBuffer(size_t new_size) = 0;
		int bindToShader(GLuint binding_index, ShaderBufferType buffer_option, GLintptr offset);
		int bindToVertexArrayObject(GLuint binding_index, GLuint vao, GLintptr offset);
		void setFence(size_t fence_index);
		void setFence() override;
		void waitFence(size_t fence_index);
		void incrementFence();
	protected:
		GLuint buffer_id = 0;
		size_t current_objs = 0, current_fence = 0, max_objs;
		std::unique_ptr<GLsyncWrap[]> fences;
		std::vector<GLuint> bound_vaos;
		std::vector<ShaderBufferType> bound_buffers;
		GLintptr internal_offset = 0;

		BufferBase(size_t max_objs, size_t max_fences, BufferAccess access, bool include_size);
		void updateBindings(GLuint new_buffer_id);
	};

	/**
	* Buffer that is mapped to client-side memory
	*/
	template<class T> class MappedBuffer : public BufferBase<T> {
	public:
		MappedBuffer(size_t max_objs, size_t max_fences, BufferAccess access, const std::vector<MappedBufferAccess>& map_access, bool include_size = false);
		~MappedBuffer();

		void updateBuffer(const std::vector<T>& data) override;
		void resizeBuffer(size_t new_size) override;
		std::span<GLchar> bufferPtr(size_t fence_index);
	private:
		GLchar* buffer_ptr = nullptr;
		GLuint mapped_access_bitfield;
	};

	/**
	* Buffer that is not mapped to client-side memory and can only be accessed by GPU commands
	*/
	template<class T> class UnmappedBuffer : public BufferBase<T> {
	public:
		UnmappedBuffer(size_t max_objs, size_t max_fences, BufferAccess access, bool include_size = false);
		~UnmappedBuffer();

		void updateBuffer(const std::vector<T>& data) override;
		void resizeBuffer(size_t new_size) override;
	};

	/**
	* Templateless class that allows tracking of ObjectBuffer instances
	*/
	class ObjectBufferWrapper {
	public:
		virtual void updateBuffer() = 0;
		inline bool isObjectsChanged() const { return objects_changed; }
	protected:
		bool objects_changed = false;
	};

	/**
	* Sychronises a client-side store of objects, and an equivalent GPU-side buffer
	*/
	template<class C, class G, GLuint DIVISIONS = 0> class ObjectBuffer : public ObjectBufferWrapper {
	public:
		ObjectBuffer(size_t max_objs, const std::function<std::array<G, DIVISIONS + 1>(const C&)>& converter, BufferAccess access, bool include_size = false);

		const std::vector<C>& getAllObjects() const;
		const C& getObject(size_t index) const;
		const C& peekObject() const;
		void pushObject(const C& obj);
		void setObject(const C& obj, size_t index);
		template<typename T> void setObjectMember(const T& value, size_t offset, size_t index);
		void insertObject(const C& obj, size_t index);
		C popObject();
		C removeObject(size_t index);
		void clearObjects();
		void setAllObjects(const std::vector<C>& objs);
		std::vector<C>& changeObjects();

		const std::unique_ptr<BufferBase<G>>& getBuffer() const;
		void bindToShader(GLuint binding_index, ShaderBufferType buffer_option, GLintptr offset = 0);
		void bindToVertexArrayObject(GLuint binding_index, GLuint vao, GLintptr offset = 0);
		void updateBuffer() override;
		void setFence();
		void waitFence();

		auto begin() const;
		auto end() const;
	private:
		std::function<std::array<G, DIVISIONS + 1>(const C&)> converter;

		std::vector<C> objs;
		std::unique_ptr<BufferBase<G>> buffer;
	};

	/**
	* Updates every buffer attached to a vertex array object.
	* 
	* @param vao Vertex array object ID
	*/
	static void updateAttachedBuffers(GLuint vao);
	/**
	* Updates every buffer of a particular buffer type at each of the given buffer locations.
	*
	* @param buffer_type Type of buffer object.
	* @param buffer_locations Buffer binding locations for the given buffer type.
	*/
	static void updateAttachedBuffers(ShaderBufferType buffer_type, const std::vector<int>& buffer_locations);
	/**
	* Set fences for each buffer attached to a vertex array object.
	* 
	* @param vao Vertex array object ID
	*/
	static void fenceAttachedBuffers(GLuint vao);
	/**
	* Set fences for each buffer attached at the given buffer locations for a particular buffer type.
	* 
	* @param buffer_type Type of buffer object.
	* @param buffer_locations Buffer binding locations for the given buffer type.
	*/
	static void fenceAttachedBuffers(ShaderBufferType buffer_type, const std::vector<int>& buffer_locations);
	/**
	* Creates a buffer of type T
	*/
	template<OpenGLObjectType::Concept T> static GLuint createBuffer();
	/**
	* Deletes the buffer with the passed ID and of type T
	* 
	* @param ID The ID of the buffer to be deleted
	*/
	template<OpenGLObjectType::Concept T> static void deleteBuffer(GLuint ID);
	/**
	* Check if buffer of BufferType T exists with buffer handle ID
	* 
	* @param ID The ID of the buffer to check
	*/
	template<OpenGLObjectType::Concept T> static bool isBuffer(GLuint ID);
	/**
	* Clears all buffers
	*/
	static void clear();

	static ShaderBufferType interfaceToBufferType(ShaderBufferInterfaceType interface_type);
private:
	/**
	* Binding data for ObjectBuffer instances
	*/
	struct BindingData {
		BufferBaseWrapper* buffer_ptr = nullptr;
		ObjectBufferWrapper* object_ptr = nullptr;
		GLuint binding_index, buffer_id;
		GLintptr offset;
	};

	static inline std::vector<std::unique_ptr<OpenGLHandleWrapper>> buffers;
	static inline std::map<GLuint, std::vector<BindingData>> vao_binding_indices;
	static inline std::map<GLuint, std::vector<BindingData>> shader_binding_indices;
};

#include "ResourceManagerClass.inl"
