#pragma once

#include <list>
#include <string>
#include <unordered_map>
#include <concepts>
#include <variant>
#include <iostream>
#include <algorithm>
#include <glm/glm.hpp>
#include <glad/glad.h>

/**
* Structs for storing OpenGL buffer IDs
*/
class OpenGLHandle {
public:
	const GLuint& getID() const;
protected:
	GLuint ID = 0;
};

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
* Concept for grouping buffer structs
*/
template<class T> concept BufferType = std::derived_from<T, OpenGLHandle>;

/**
* Manages OpenGL buffer objects
*/
class TAGResourceManager {
public:
	/**
	* The base path from the executable to loadable assets
	*/
	static inline std::string asset_path = "../assets/";

	/**
	* Interface for implementing buffer handling structs
	*/
	template<class T> class BufferHandler {
	public:
		virtual ~BufferHandler() = default;
		BufferHandler(const BufferHandler&) = delete;
		BufferHandler& operator=(const BufferHandler&) = delete;

		unsigned int getMaxObjects() const;
		unsigned int getCurrentObjects() const;
		BufferAccess getAccessLevel() const;
		virtual void bindBuffer(const GLuint& binding_index, const GLuint& vao = 0) const = 0;
		virtual void updateBuffer(const std::vector<T>& data) = 0;
		virtual void resizeBuffer(const unsigned int& new_size) = 0;
	protected:
		BufferAccess access;
		GLuint buffer_id;
		unsigned int max_objs;
		unsigned int current_objs = 0;
	};

	/**
	* Struct for handling ring buffers, for STREAM level buffers
	*/
	template<class T, unsigned int MAX_FENCES> class RingBuffer : public BufferHandler<T> {
		static_assert(MAX_FENCES > 1 && MAX_FENCES < 6, "MAX_FENCES must be between 2 and 5");
	public:
		RingBuffer(const unsigned int& max_objs);
		~RingBuffer();
		void updateBuffer(const std::vector<T>& data) override;
		void bindBuffer(const GLuint& binding_index, const GLuint& vao = 0) const override;
		void resizeBuffer(const unsigned int& new_size) override;
		void setFence();
	private:
		T* buffer_ptr = nullptr;
		std::array<GLsync, MAX_FENCES> fences = {};
		unsigned int current_fence = 0;
	};

	/**
	* Struct for handling orphan buffers, for DYNAMIC and STATIC level buffers
	*/
	template<class T> class OrphanBuffer : public BufferHandler<T> {
	public:
		OrphanBuffer(const unsigned int& max_objs, const BufferAccess& access);
		~OrphanBuffer();
		void updateBuffer(const std::vector<T>& data) override;
		void bindBuffer(const GLuint& binding_index, const GLuint& vao = 0) const override;
		void resizeBuffer(const unsigned int& new_size) override;
	};

	/**
	* Struct for handling buffer access
	*/
	template<class C, class G> class ObjectBuffer {
	public:
		ObjectBuffer(const unsigned int& max_objs, G(*converter)(const C&), const BufferAccess& access);

		const std::vector<C>& getObjects() const;
		std::vector<C>& changeObjects();
		bool isObjectsChanged() const;

		const BufferHandler<G>& getBuffer() const;
		void updateBuffer();
		void resizeBuffer(const unsigned int& new_size);

		void changeAccess(const BufferAccess& new_access);
	private:
		G(*converter)(const C&);

		bool objects_changed = false;
		std::vector<C> objs;
		std::unique_ptr<BufferHandler<G>> buffer;
	};

	/**
	* Deletes the buffer with the passed ID and of type T
	* 
	* @param ID The ID of the buffer to be deleted
	*/
	template<BufferType T> static void deleteBuffer(const GLuint& ID);
	/**
	* Creates a buffer of type T
	*/
	template<BufferType T> static GLuint createBuffer();
	/**
	* Check if buffer of BufferType T exists with buffer handle ID
	* 
	* @param ID The ID of the buffer to check
	*/
	template<BufferType T> static bool isBuffer(const GLuint& ID);
	/**
	* Clears all buffers
	*/
	static void clear();
private:
	using BufferVariant = std::variant<VertexArrayObject, ProgramShader, VertexShader, FragmentShader, TextureBuffer, GenericBuffer>;
	static inline std::list<BufferVariant> buffers;
};

#include "../../src/ResourceManagerClass.inl"