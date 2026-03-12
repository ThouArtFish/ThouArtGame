#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ResourceManagerClass.hpp"
#include "UtilClass.hpp"

/**
 * Manages in-game lights. Stores Light structs in a vector for client-side access and also controls a shader storage buffer object
 * to store the lights GPU-side.
 */
class TAGLightManager {
	public:
		/**
		 * Convoluted way of representing multiple lighting types to cram as much information as possible into two 4D vectors.
		 * The light can be point, directional or flashlight.
		 * The formatting for each light is the following:
		 * 
		 * a.xyz = position (flash, point); = direction (direc)
		 * b.xyz = color (point, direc); = direction (flash)
		 * a.w = distance factor
		 * b.w > 0 (cone scope for flash in radians); = 0 (declares light is point); < 0 (declares light is direc)
		 * 
		 * Yes flashlights can have only a single light colour.
		 * This needs to be overhauled immediately.
		 */
		struct Light {
			glm::vec4 a;
			glm::vec4 b;
		};
		/**
		 * Describes how often the lights are changed. This is not a hard restriction and just improves the efficiency of data traversal.
		 * STATIC means data changes basically never. DYNAMIC is for sometimes. STREAM if changes happen every frame.
		 */
		enum class ChangeFreq : GLenum {
			STATIC = GL_STATIC_DRAW,
			DYNAMIC = GL_DYNAMIC_DRAW,
			STREAM = GL_STREAM_DRAW
		};
		bool delete_on_death = true;

		/**
		 * Light buffer is initialized with no data.
		 * The initial size of the buffer is not the hard limit and will dynamically change in size if 
		 * more lights are added.
		 * 
		 * @params size The size of the light buffer.
		 * @params chnage_freq How often the buffer is changed.
		 */
		TAGLightManager(const unsigned int& size, const ChangeFreq& change_freq);
		/**
		 * Light buffer is initialized with light data and at max size.
		 * The initial size of the buffer is not the hard limit and will dynamically change in size if
		 * more lights are added.
		 *
		 * @params lights The lights to push to the buffer.
		 * @params chnage_freq How often the buffer is changed.
		 */
		TAGLightManager(const std::vector<Light>& lights, const ChangeFreq& change_freq);
		~TAGLightManager();
		/**
		 * Returns all lights, but allows changes.
		 */
		std::vector<Light>& changeLights();
		/**
		 * Returns all lights.
		 */
		const std::vector<Light>& getLights() const;
		/**
		 * Binds the light buffer to all shaders at binding point index.
		 * 
		 * @param index Binding point os shader storage buffer object in any shader.
		 */
		void bindShaderData(const unsigned int& index);
		/**
		 * Unbinds the light buffer from all shaders at binding point index.
		 * 
		 * @param index Binding point of shader storage buffer object in any shader.
		 */
		void unbindShaderData(const unsigned int& index) const;
		/**
		 * Returns iterator for traversing lights
		 */
		auto begin() const;
		/**
		 * Returns iterator for traversing lights
		 */
		auto end() const;
		/**
		 * Returns number of lights
		 */
		unsigned int size() const;
	private:
		unsigned int buffer_size;
		unsigned int buffer_ID = 0;
		bool was_updated = true;
		std::vector<Light> lights;
};
