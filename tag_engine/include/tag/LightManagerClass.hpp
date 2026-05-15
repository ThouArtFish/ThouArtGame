#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ResourceManagerClass.hpp"
#include "UtilClass.hpp"

/**
* Concept for allowing only the light structs
*/
template<class T> concept LightType = isAnyOf<T, TAGLightManager::PointLight, TAGLightManager::RayLight, TAGLightManager::FlashLight>;

/**
 * Manages in-game lights. Stores Light structs in a vector for client-side access and also controls a shader storage buffer object
 * to store the lights GPU-side.
 */
class TAGLightManager {
	public:
		struct PointLight {
			glm::vec4 position;
			glm::vec4 colour;
		};

		struct RayLight {
			glm::vec4 direction;
			glm::vec4 colour;
		};

		struct FlashLight {
			glm::vec4 position;
			glm::vec4 direction;
			glm::vec4 colour;
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
