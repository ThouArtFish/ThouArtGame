#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "BaseStateClass.hpp"
#include "ResourceManagerClass.hpp"
#include "UtilClass.hpp"

namespace TAGLight {
	/**
	* Client side light structs
	*/
	struct Point {
		glm::vec3 position, colour;
		glm::vec2 attenuation;
	};

	struct Ray {
		glm::vec3 direction, colour;
	};

	struct Flash {
		glm::vec3 position, direction, colour;
		glm::vec2 attenuation;
		float angle;
	};

	/**
	* Shader side light structs
	*/
	struct alignas(16) ShaderPoint {
		glm::vec4 a, b;
	};

	struct alignas(16) ShaderRay {
		glm::vec4 a;
		glm::vec2 b;
	};

	struct alignas(16) ShaderFlash {
		glm::vec4 a, b, c;
	};
};

/**
* Names for light type members when setting individual attributes
*/
namespace PointLightMemberName {
	struct TagStruct {};
	struct POSITION : TagStruct { using TYPE = glm::vec3; static constexpr std::size_t OFFSET = offsetof(TAGLight::Point, position); };
	struct COLOUR : TagStruct { using TYPE = glm::vec3; static constexpr std::size_t OFFSET = offsetof(TAGLight::Point, colour); };
	struct ATTENUATION : TagStruct { using TYPE = glm::vec2; static constexpr std::size_t OFFSET = offsetof(TAGLight::Point, attenuation); };

	template<typename T> concept Concept = !std::same_as<T, TagStruct> && std::derived_from<T, TagStruct>;
};

namespace RayLightMemberName {
	struct TagStruct {};
	struct DIRECTION : TagStruct { using TYPE = glm::vec3; static constexpr std::size_t OFFSET = offsetof(TAGLight::Ray, direction); };
	struct COLOUR : TagStruct { using TYPE = glm::vec3; static constexpr std::size_t OFFSET = offsetof(TAGLight::Ray, colour); };

	template<typename T> concept Concept = !std::same_as<T, TagStruct> && std::derived_from<T, TagStruct>;
};

namespace FlashLightMemberName {
	struct TagStruct {};
	struct POSITION : TagStruct { using TYPE = glm::vec3; static constexpr std::size_t OFFSET = offsetof(TAGLight::Flash, position); };
	struct DIRECTION : TagStruct { using TYPE = glm::vec3; static constexpr std::size_t OFFSET = offsetof(TAGLight::Flash, direction); };
	struct COLOUR : TagStruct { using TYPE = glm::vec3; static constexpr std::size_t OFFSET = offsetof(TAGLight::Flash, colour); };
	struct ATTENUATION : TagStruct { using TYPE = glm::vec2; static constexpr std::size_t OFFSET = offsetof(TAGLight::Flash, attenuation); };
	struct ANGLE : TagStruct { using TYPE = float; static constexpr std::size_t OFFSET = offsetof(TAGLight::Flash, angle); };

	template<typename T> concept Concept = !std::same_as<T, TagStruct> && std::derived_from<T, TagStruct>;
};

/**
* Concept for checking if light struct member name is correct, depending on the type of light the current instance of TAGLightManager is storing
*/
namespace VariableLightMemberName {
	template<typename L, typename V> concept Concept =
		(std::same_as<L, TAGLight::Point> && PointLightMemberName::Concept<V>) ||
		(std::same_as<L, TAGLight::Ray> && RayLightMemberName::Concept<V>) ||
		(std::same_as<L, TAGLight::Flash> && FlashLightMemberName::Concept<V>);
};

/**
* Concept for allowing only the light structs
*/
template<class T> concept LightType = isAnyOf<T, TAGLight::Point, TAGLight::Ray, TAGLight::Flash>;

/**
* Struct for extracting shader type of light
*/
template<LightType T> struct ShaderLightType;
template<> struct ShaderLightType<TAGLight::Point> { using TYPE = TAGLight::ShaderPoint; static constexpr GLuint default_binding_point = 0; };
template<> struct ShaderLightType<TAGLight::Ray> { using TYPE = TAGLight::ShaderRay; static constexpr GLuint default_binding_point = 1; };
template<> struct ShaderLightType<TAGLight::Flash> { using TYPE = TAGLight::ShaderFlash; static constexpr GLuint default_binding_point = 2; };

/**
* Manages in-game lights. Stores Light structs in a vector for client-side access and also controls a shader storage buffer object
* to store the lights GPU-side.
*/
template<LightType T> class TAGLightManager : public TAGBaseState::OpenGLContextChecker {
	public:
		static inline constexpr GLuint default_scene_binding_point = 3;
		
		static inline constexpr GLuint MAX_BINDING_INDEX = 6;

		/**
		* Scene data for shaders
		*/
		struct Scene {
			GLfloat ambience;
		};

		using ShaderT = ShaderLightType<T>::TYPE;
		using SceneBufferObject = TAGResourceManager::ObjectBuffer<Scene, GLfloat>;

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
		* Set an attribute of an existing light struct, based on the offset of the attribute in the Light struct
		*
		* @param value Value to set
		* @param index Index in array
		*/
		template<typename V> requires VariableLightMemberName::Concept<T, V> void setLightMember(const V::TYPE& value, const GLuint& index);
		/**
		* Removes light at index.
		* Pops last light if no index is passed.
		* A buffer update function must be used for changes to be reflected in the GPU buffer.
		* 
		* @param index Index in light array
		*/
		T removeLight(const int& index = -1);
		/**
		* Clears all lights and sets new lights from parameter.
		* A buffer update function must be used for changes to be reflected in the GPU buffer.
		* 
		* @param lights New lights.
		*/
		void setAllLights(const std::vector<T>& lights);
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
		* Returns number of lights in GPU buffer
		*/
		unsigned int bufferSize() const;
	private:
		TAGResourceManager::ObjectBuffer<T, ShaderT> lights;
		static inline std::variant<std::monostate, SceneBufferObject> scene;

		static ShaderT shaderLightConverter(const T& light, const GLuint& split = 0);
		static GLfloat shaderSceneConverter(const Scene& scene, const GLuint& split = 0);
		static void initSceneBuffer();
};

#include "LightManagerClass.inl"
