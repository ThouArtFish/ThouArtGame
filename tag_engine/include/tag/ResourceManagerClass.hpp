#pragma once

#include <list>
#include <string>
#include <unordered_map>
#include <concepts>
#include <variant>
#include <iostream>
#include <algorithm>
#include <glad/glad.h>

/**
* Structs for storing OpenGL buffer IDs
*/
struct OpenGLVertexArrayObject {
	OpenGLVertexArrayObject();
	~OpenGLVertexArrayObject();
	unsigned int ID;
};
struct OpenGLProgram {
	OpenGLProgram();
	~OpenGLProgram();
	unsigned int ID;
};
struct OpenGLVertexShader {
	OpenGLVertexShader();
	~OpenGLVertexShader();
	unsigned int ID;
};
struct OpenGLFragmentShader {
	OpenGLFragmentShader();
	~OpenGLFragmentShader();
	unsigned int ID;
};
struct OpenGLTexture {
	OpenGLTexture();
	~OpenGLTexture();
	unsigned int ID;
};
struct OpenGLBuffer {
	OpenGLBuffer();
	~OpenGLBuffer();
	unsigned int ID;
};

/**
* Concept for grouping buffer structs
*/
template<class T> concept BufferType =
std::same_as<T, OpenGLVertexArrayObject>
|| std::same_as<T, OpenGLTexture>
|| std::same_as<T, OpenGLBuffer>
|| std::same_as<T, OpenGLProgram>
|| std::same_as<T, OpenGLFragmentShader>
|| std::same_as<T, OpenGLVertexShader>;

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
	* Deletes the buffer with the passed ID and of type T
	* 
	* @param ID The ID of the buffer to be deleted
	*/
	template<BufferType T> static void deleteBuffer(const unsigned int& ID);
	/**
	* Creates a buffer of type T
	*/
	template<BufferType T> static unsigned int createBuffer();
	/**
	* Clears all buffers
	*/
	static void clear();
private:
	using BufferVariant = std::variant<
		OpenGLTexture, OpenGLVertexArrayObject, OpenGLBuffer, OpenGLProgram, OpenGLVertexShader, OpenGLFragmentShader
	>;

	static inline std::list<BufferVariant> buffers;
};

#include "../../src/ResourceManagerClass.inl"