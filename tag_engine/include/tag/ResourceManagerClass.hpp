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

/**
* Manages OpenGL buffer objects
*/
class TAGResourceManager {
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
	enum class ShaderBuffer {
		SHADER_STORAGE = GL_SHADER_STORAGE_BUFFER,
		ATOMIC_COUNTER = GL_ATOMIC_COUNTER_BUFFER,
		UNIFORM = GL_UNIFORM_BUFFER,
		TRANSFORM_FEEDBACK = GL_TRANSFORM_FEEDBACK_BUFFER
	};

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
	public:
		virtual ~BufferHandler() = default;
		BufferHandler(const BufferHandler&) = delete;
		BufferHandler& operator=(const BufferHandler&) = delete;

		const BufferAccess access;

		GLuint getMaxObjects() const;
		GLuint getCurrentObjects() const;
		void bindToShader(const GLuint& binding_index, const GLintptr& offset, const ShaderBuffer& buffer_option);
		void bindToVertexArrayObject(const GLuint& binding_index, const GLintptr& offset, const GLuint& vao);
		void setFence();
	protected:
		const GLuint include_size;
		GLuint buffer_id, max_objs;
		std::unique_ptr<GLsyncWrap[]> fences;
		std::vector<GLuint> bound_vaos;
		std::vector<ShaderBuffer> bound_buffers;
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
		friend class ObjectBuffer;
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
		friend class ObjectBuffer;
	public:
		OrphanBuffer(const GLuint& max_objs, const BufferAccess& access, const bool& include_size = false);
		~OrphanBuffer();
	private:
		void updateBuffer(const std::vector<T>& data) override;
		void resizeBuffer(const GLuint& new_size) override;
	};

	/**
	* Struct for handling buffer access
	*/
	template<class C, class G, GLuint DIVISIONS = 0> class ObjectBuffer {
	public:
		ObjectBuffer(const GLuint& max_objs, G(*converter)(const C&, const GLuint&), const BufferAccess& access, const bool& include_size = false);

		const std::vector<C>& getObjects() const;
		std::vector<C>& changeObjects();
		bool isObjectsChanged() const;

		const std::unique_ptr<BufferHandler<G>>& getBuffer();
		void updateBuffer();
	private:
		G(*converter)(const C&, const GLuint&);

		bool objects_changed = false;
		std::vector<C> objs;
		std::unique_ptr<BufferHandler<G>> buffer;
	};

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
		GLuint binding_index, buffer_id;
		GLintptr offset;
	};

	using BufferVariant = std::variant<VertexArrayObject, ProgramShader, VertexShader, FragmentShader, TextureBuffer, GenericBuffer>;
	static inline std::list<BufferVariant> buffers;
	static inline std::map<GLuint, std::vector<BindingData>> vao_binding_indices;
	static inline std::map<ShaderBuffer, std::vector<BindingData>> shader_binding_indices;
};

#include "../../src/ResourceManagerClass.inl"