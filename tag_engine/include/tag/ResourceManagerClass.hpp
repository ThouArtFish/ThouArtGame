#pragma once

#include <list>
#include <string>
#include <map>
#include <concepts>
#include <variant>
#include <vector>
#include <algorithm>
#include <memory>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "ShaderManagerClass.hpp"

/**
* Manages OpenGL buffer objects
*/
class TAGResourceManager {
	template<class C, class G, GLuint DIVISIONS> friend class ObjectBuffer;
public:
	/**
	* Structs for storing OpenGL buffer IDs
	*/
	class OpenGLHandle {
	public:
		const GLuint& getID() const;
	protected:
		GLuint ID = 0;
	};

	/**
	* Buffer types
	*/
	struct VertexArrayObject : public OpenGLHandle {
		VertexArrayObject();
		~VertexArrayObject();
	};
	struct ProgramShader : public OpenGLHandle {
		ProgramShader();
		~ProgramShader();
	};
	struct VertexShader : public OpenGLHandle {
		VertexShader();
		~VertexShader();
	};
	struct FragmentShader : public OpenGLHandle {
		FragmentShader();
		~FragmentShader();
	};
	struct TextureBuffer : public OpenGLHandle {
		TextureBuffer();
		~TextureBuffer();
	};
	struct GenericBuffer : public OpenGLHandle {
		GenericBuffer();
		~GenericBuffer();
	};

	/**
	* Buffer access levels
	*/
	enum class BufferAccess {
		STATIC = GL_STATIC_DRAW,
		DYNAMIC = GL_DYNAMIC_DRAW,
		STREAM = GL_STREAM_DRAW
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
	static constexpr inline std::array<ShaderBufferType, 4> buffer_types = { ShaderBufferType::SHADER_STORAGE, ShaderBufferType::ATOMIC_COUNTER, ShaderBufferType::UNIFORM, ShaderBufferType::TRANSFORM_FEEDBACK };

	/**
	* The base path from the executable to loadable assets
	*/
	static inline std::string asset_path = "../assets/";

	/**
	* Default constructible wrapper for GLsync objects
	*/
	struct GLsyncWrap {
		GLsync sync = nullptr;
	};

	/**
	* Interface for implementing buffer handling structs
	*/
	template<class T> class BufferHandler {
		template<class C, class G, GLuint DIVISIONS> friend class ObjectBuffer;
	public:
		virtual ~BufferHandler() = default;
		BufferHandler(const BufferHandler&) = delete;
		BufferHandler& operator=(const BufferHandler&) = delete;

		const BufferAccess access;
		const GLuint include_size;

		GLuint getMaxObjects() const;
		GLuint getCurrentObjects() const;
		GLuint getBufferID() const;
		void bindToShader(const GLuint& binding_index, const GLintptr& offset, const ShaderBufferType& buffer_option);
		void bindToVertexArrayObject(const GLuint& binding_index, const GLintptr& offset, const GLuint& vao);
		void setFence();
	protected:
		GLuint buffer_id, max_objs;
		std::unique_ptr<GLsyncWrap[]> fences;
		std::vector<GLuint> bound_vaos;
		std::vector<ShaderBufferType> bound_buffers;
		GLuint current_objs = 0, current_fence = 0;
		GLintptr internal_offset = 0;

		BufferHandler(const bool& include_size, const BufferAccess& access);
		void updateBindings(const GLuint& new_buffer_id);
		virtual void updateBuffer(const std::vector<T>& data) = 0;
		virtual void resizeBuffer(const GLuint& new_size) = 0;
	};

	/**
	* Struct for handling ring buffers, for STREAM level buffers
	*/
	template<class T, GLuint MAX_FENCES> class RingBuffer : public BufferHandler<T> {
		static_assert(MAX_FENCES > 1 && MAX_FENCES < 6, "MAX_FENCES must be between 2 and 5");
		template<class C, class G, GLuint DIVISIONS> friend class ObjectBuffer;
	public:
		RingBuffer(const GLuint& max_objs, const bool& include_size = false);
		~RingBuffer();
	private:
		GLchar* buffer_ptr = nullptr;

		void updateBuffer(const std::vector<T>& data) override;
		void resizeBuffer(const GLuint& new_size) override;
	};

	/**
	* Struct for handling orphan buffers, for DYNAMIC and STATIC level buffers
	*/
	template<class T> class OrphanBuffer : public BufferHandler<T> {
		template<class C, class G, GLuint DIVISIONS> friend class ObjectBuffer;
	public:
		OrphanBuffer(const GLuint& max_objs, const BufferAccess& access, const bool& include_size = false);
		~OrphanBuffer();
	private:
		void updateBuffer(const std::vector<T>& data) override;
		void resizeBuffer(const GLuint& new_size) override;
	};

	/**
	* Wrapper to allow instance tracking in a map
	*/
	class ObjectBufferWrapper {
	public:
		virtual ~ObjectBufferWrapper() = default;
		ObjectBufferWrapper(const ObjectBufferWrapper&) = delete;
		ObjectBufferWrapper& operator=(const ObjectBufferWrapper&) = delete;

		virtual void updateBuffer() = 0;
		bool isObjectsChanged() const;
	protected:
		bool objects_changed = false;
	};

	/**
	* Struct for handling buffer access
	*/
	template<class C, class G, GLuint DIVISIONS = 0> class ObjectBuffer : public ObjectBufferWrapper {
	public:
		ObjectBuffer(const GLuint& max_objs, G(*converter)(const C&, const GLuint&), const BufferAccess& access, const bool& include_size = false);
		~ObjectBuffer();

		const std::vector<C>& getAllObjects() const;
		const C& getObject(const GLuint& index) const;
		const C& peekObject() const;
		void pushObject(const C& obj);
		void setObject(const C& obj, const GLuint& index);
		void insertObject(const C& obj, const GLuint& index);
		C popObject();
		C removeObject(const GLuint& index);
		void setAllObjects(const std::vector<C>& objs);
		std::vector<C>& changeObjects();

		const std::unique_ptr<BufferHandler<G>>& getBuffer();
		void updateBuffer() override;

		auto begin() const;
		auto end() const;
	private:
		G(*converter)(const C&, const GLuint&);

		std::vector<C> objs;
		std::unique_ptr<BufferHandler<G>> buffer;
	};

	/**
	* Updates every buffer attached to a vertex array object, if it requires an update to its contents.
	* 
	* @param vao Vertex array object ID
	*/
	static void updateAttachedBuffers(const GLuint& vao);
	/**
	* Updates every buffer attached to any shader of the buffer type, if it requires an update to its contents.
	* 
	* @param buffer_type Type of buffer object to update instances of.
	*/
	static void updateAttachedBuffers(const ShaderBufferType& buffer_type);
	/**
	* Updates every buffer attached to any shader of any buffer type, if it requires an update to its contents.
	*/
	static void updateAttachedBuffers();
	/**
	* Updates every buffer that a vertex array object contains a binding index for, if it exists and requires an update to its contents
	*
	* @param vao VAO ID.
	*/
	static void updateReferencedBuffers(const GLuint& vao);
	/**
	* Updates every buffer of a particular buffer type a shader contains a binding index for, if it exists and requires an update to its contents
	* 
	* @param buffer_type Type of buffer object to update instances of.
	* @param shader Shader object.
	*/
	static void updateReferencedBuffers(const ShaderBufferType& buffer_type, const TAGShaderManager::Shader& shader);
	/**
	* Updates every buffer that a shader contains a binding index for, if it exists and requires an update to its contents
	*
	* @param shader Shader object.
	*/
	static void updateReferencedBuffers(const TAGShaderManager::Shader& shader);
	/**
	* Deletes the buffer with the passed ID and of type T
	* 
	* @param ID The ID of the buffer to be deleted
	*/
	template<class T> requires std::derived_from<T, TAGResourceManager::OpenGLHandle> static void deleteBuffer(const GLuint& ID);
	/**
	* Creates a buffer of type T
	*/
	template<class T> requires std::derived_from<T, TAGResourceManager::OpenGLHandle> static GLuint createBuffer();
	/**
	* Check if buffer of BufferType T exists with buffer handle ID
	* 
	* @param ID The ID of the buffer to check
	*/
	template<class T> requires std::derived_from<T, TAGResourceManager::OpenGLHandle> static bool isBuffer(const GLuint& ID);
	/**
	* Clears all buffers
	*/
	static void clear();
private:
	struct BindingData {
		ObjectBufferWrapper* ptr = nullptr;
		GLuint binding_index, buffer_id = 0;
		GLintptr offset = 0;
	};

	using BufferVariant = std::variant<VertexArrayObject, ProgramShader, VertexShader, FragmentShader, TextureBuffer, GenericBuffer>;
	static inline std::list<BufferVariant> buffers;
	static inline std::map<GLuint, std::vector<BindingData>> vao_binding_indices;
	static inline std::map<ShaderBufferType, std::vector<BindingData>> shader_binding_indices;
};

#include "../../src/ResourceManagerClass.inl"
