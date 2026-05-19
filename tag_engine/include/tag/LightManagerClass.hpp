#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ResourceManagerClass.hpp"
#include "UtilClass.hpp"

/**
* Client side light structs
*/
struct PointLight {
	glm::vec3 position, colour;
	glm::vec2 attenuation;
};

struct RayLight {
	glm::vec3 direction, colour;
};

struct FlashLight {
	glm::vec3 position, direction, colour;
	glm::vec2 attenuation;
	float angle;
};

/**
* Shader side light structs
*/
struct alignas(16) ShaderPointLight {
	glm::vec4 a, b;
};

struct alignas(16) ShaderRayLight {
	glm::vec4 a;
	glm::vec2 b;
};

struct alignas(16) ShaderFlashLight {
	glm::vec4 a, b, c;
};

/**
* Concept for allowing only the light structs
*/
template<class T> concept LightType = isAnyOf<T, PointLight, RayLight, FlashLight>;

/**
* Struct for extracting shader type of light
*/
template<LightType T> struct ShaderLightType { using type = T; static inline GLuint default_binding_point = 0; };
template<> struct ShaderLightType<PointLight> { using type = ShaderPointLight; static inline GLuint default_binding_point = 0; };
template<> struct ShaderLightType<RayLight> { using type = ShaderRayLight; static inline GLuint default_binding_point = 1; };
template<> struct ShaderLightType<FlashLight> { using type = ShaderFlashLight; static inline GLuint default_binding_point = 2; };

/**
 * Manages in-game lights. Stores Light structs in a vector for client-side access and also controls a shader storage buffer object
 * to store the lights GPU-side.
 */
template<LightType T> class TAGLightManager {
	public:
		bool delete_on_death = true;

		static inline GLuint default_scene_binding_point = 3;
		
		static inline constexpr GLuint MAX_BINDING_INDEX = 6;

		/**
		* Scene data for shaders
		*/
		struct Scene {
			GLfloat ambience;
		};

		using ShaderT = ShaderLightType<T>::type;
		using SceneObject = TAGResourceManager::ObjectBuffer<Scene, GLfloat>;

		/**
		 * Initialize with a number of lights and access manager
		 * 
		 * @params lights Array of lights
		 * @param access Access modifier for stored lights
		 * @param size Size of buffer
		 */
		TAGLightManager(const std::vector<T>& lights, const TAGResourceManager::BufferAccess& access, const unsigned int& size = 1);
		/**
		 * Initialize by allocating a size for buffer and access manager
		 * 
		 * @param access Access modifier for stored lights
		 * @params size Size of buffer
		 */
		TAGLightManager(const TAGResourceManager::BufferAccess& access, const unsigned int& size = 1);

		/**
		 * Set light at index.
		 * Pushes to end of light array if no index is passed.
		 * A buffer update function must be used for changes to be reflected in the GPU buffer.
		 * 
		 * @param light New light
		 * @param index Index in light array
		 */
		void setLight(const T& light, const int& index = -1);
		/**
		 * Get light at index.
		 * Gets last light if no index is passed.
		 * 
		 * @param index Index in light array
		 */
		const T& getLight(const int& index = -1) const;
		/**
		* Get all lights.
		*/
		const std::vector<T>& getAllLights() const;
		/**
		 * Binds the light buffer to all shaders at binding point index.
		 * Also update GPU side buffer if update is required.
		 * 
		 * @param index Binding point of shader storage buffer object in any shader.
		 */
		void bindToShader(const GLintptr& offset = 0, const GLuint& index = ShaderLightType<T>::default_binding_point);
		/**
		* Updates GPU side buffer with CPU side lights.
		*/
		void updateLightBuffer();
		/**
		* Set scene data
		*
		* @param scene New scene data.
		*/
		static void setScene(const Scene& scene);
		/**
		* Get scene data.
		*/
		static const Scene& getScene();
		/**
		* Set scene data to all shaders at binding point index.
		*
		* @param index Binding point of shader storage buffer object in any shader.
		*/
		static void bindSceneToShader(const GLuint& index = default_scene_binding_point);
		/**
		 * Returns iterator for traversing lights
		 */
		auto begin() const;
		/**
		 * Returns iterator for traversing lights
		 */
		auto end() const;
		/**
		* Returns number of lights in CPU array
		*/
		unsigned int size() const;
		/**
		 * Returns number of lights in GPU buffer
		 */
		unsigned int bufferSize() const;
	private:
		TAGResourceManager::ObjectBuffer<T, ShaderT> lights;
		static std::variant<std::monostate, SceneObject> scene;

		static ShaderT shaderLightConverter(const T& light, const unsigned int& split = 0);
		static GLfloat shaderSceneConverter(const Scene& scene, const unsigned int& split = 0);
		static void initSceneBuffer();
};

#include "../../src/LightManagerClass.inl"