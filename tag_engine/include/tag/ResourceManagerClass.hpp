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
	* Struct for handling triple ring buffers
	*/
	template<class T, unsigned int MAX_FENCES> class RingBuffer {
		static_assert(MAX_FENCES > 0 && MAX_FENCES < 11, "MAX_FENCES must be between 1 and 10");
	public:
		RingBuffer(const unsigned int& max_objs);
		~RingBuffer();
		void updateBuffer(const std::vector<T>& data);
		void bindBuffer(const GLuint& binding_index, const GLuint& vao = 0) const;
		void resizeBuffer(const unsigned int& new_size);
		void setFence();
		const unsigned int& getMaxObjects() const;
	private:
		T* buffer_ptr = nullptr;
		std::array<GLsync, MAX_FENCES> fences = {};
		GLuint buffer_id = 0;
		unsigned int current_fence = 0;
		unsigned int current_objs = 0;
		unsigned int max_objs;
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