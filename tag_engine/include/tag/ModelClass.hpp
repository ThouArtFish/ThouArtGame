#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <tinyobjloader/tiny_obj_loader.h>
#include "ResourceManagerClass.hpp"
#include "BaseStateClass.hpp"
#include "TextureLoaderClass.hpp"
#include "ShaderManagerClass.hpp"
#include "MeshClass.hpp"

namespace ObjectMemberName {
	struct TagStruct {};
	struct POSITION;
	struct ROTATION_AXIS;
	struct ANGLE;
	struct SCALE;

	template<typename T> concept Concept = !std::same_as<T, TagStruct> && std::derived_from<T, TagStruct>;
};

/**
 * Stores the contents of the "model" at the path passed to the constructor, which can consist of many meshes.
 * Handles in-game instances of meshes through the Object struct and its associated functions.
 * Many functions have the feature to not pass a mesh name, which means the function is handling an instance of
 * stored meshes, instead of just one mesh, although it could be one mesh.
 */
class TAGModel : public TAGBaseState::OpenGLContextChecker {
public:
	/**
	* Default binding location for object buffers in instanced drawing
	*/
	static inline GLuint default_vao_instance_binding_point = 1;

	/**
	* Default size allocation for instances in buffer
	*/
	static inline GLuint default_instance_buffer_size = 20;

	/**
	 * Container for various info describing an instance of a mesh in-game.
	 */
	struct Object {
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 rotation_axis = glm::vec3(0.0f, 1.0f, 0.0f);
		float angle = 0.0f;
		float scale = 1.0f;
	};

	/**
	* GPU representation of objects
	*/
	struct alignas(16) ShaderObject {
		glm::vec4 position_AND_scale;
		glm::vec4 axis_AND_rotation;
	};

	/**
	 * Path to the model file, and parameters for texture loading.
	 * Path can be empty, so TAGMesh can be added later.
	 * Access specifies the frequency at which instances are changed.
	 *  
	 * @param params Parameters for texture loading.
	 * @param access Frequency of changes.
	 * @param path Path to model file.
	 */
	TAGModel(const TAGTexLoader::Params& params, const TAGResourceManager::BufferAccess& access, const std::string& path = "");
	TAGModel(const TAGModel&) = delete;
	TAGModel& operator=(const TAGModel&) = delete;

	/**
	 * Draws all instances of the specified mesh, or all instances which represent all meshes if no mesh name is passed.
	 * Also updates instance buffers if they are being used and require updates.
	 * 
	 * @param shader Shader to draw with.
	 * @param mesh_name Name of mesh to draw all instances of, can be left as default to draw all instances of the entire model
	 * @param options Names of shader uniforms, can be left as default for default shader uniform names
	 */
	void drawAll(const TAGShaderManager::Shader& shader, const std::string& mesh_name = "", const TAGShaderManager::ShaderOptions& options = TAGShaderManager::default_options);
	/**
	 * Draws one instance of a specified mesh, or one instance of all meshes if no mesh name is passed.
	 * 
	 * @param shader Shader to draw with
	 * @param obj Instance of mesh to draw
	 * @param mesh_name Name of mesh to draw all instances of, can be left as default to draw all instances of the entire model
	 * @param options Names of shader uniforms, can be left as default for default shader uniform names
	 */
	void drawOne(const TAGShaderManager::Shader& shader, const Object& obj, const std::string& mesh_name = "", const TAGShaderManager::ShaderOptions& options = TAGShaderManager::default_options);
	/**
	* Set an object for a particular mesh, or all meshes if no mesh_name is given.
	* Index must be less than the object count, or pushes to back if no index is passed.
	*  
	* @param obj Instance to set.
	* @param index Index in internal instance array to set to.
	* @param mesh_name Name of mesh to set instance of.
	*/
	void setInstance(const Object& obj, const int& index = -1, const std::string& mesh_name = "");
	/**
	* Set an attribute of an existing instance struct, based on the offset of the attribute in the Object struct
	*
	* @param value Value to set
	* @param index Index in array
	* @param mesh_name Name of mesh
	*/
	template<ObjectMemberName::Concept T> void setInstanceMember(const T::TYPE& value, const GLuint& index, const std::string& mesh_name = "");
	/**
	* Removes instance of a particular mesh, or an instance of all meshes if no mesh_name is given.
	* Index must be less than the object count, pops last instance if no index is passed.
	*
	* @param index Index in light array
	*/
	Object removeInstance(const int& index = -1, const std::string& mesh_name = "");
	/**
	* Clears all instances of a particular mesh, or all instances of all meshes if no mesh_name is given, and sets new lights from parameter.
	* 
	* @param objs New instances.
	* @param mesh_name Name of mesh.
	*/
	void setAllInstances(const std::vector<Object>& objs, const std::string& mesh_name = "");
	/**
	* Get an object for a particular mesh, or all meshes if no mesh_name is given.
	* Index must be less than the object count, or -1 to get last object.
	*
	* @param index Index in internal instance array to get from.
	* @param mesh_name Name of mesh to get instance of.
	*/
	const Object& getInstance(const int& index = -1, const std::string& mesh_name = "") const;
	/**
	* Get all instances of mesh, or all instances that represent every mesh if no mesh_name is passed.
	* 
	* @param mesh_name Name of mesh.
	*/
	const std::vector<Object>& getAllInstances(const std::string& mesh_name = "") const;
	/**
	* Updates GPU side buffer with CPU side instances.
	* 
	* @param mesh_name Name of mesh to update instances for, or the instances that represent every mesh if no mesh_name is passed.
	*/
	void updateInstanceBuffer(const std::string& mesh_name);
	/**
	 * Access mesh.
	 * 
	 * @param mesh_name Name of mesh.
	 */
	TAGMesh& getMesh(const std::string& mesh_name);
	/**
	 * Get all mesh names.
	 */
	std::vector<std::string> getMeshNames() const;
	/**
	* Add a mesh.
	* 
	* @param mesh_name Name of mesh
	* @param vertices Vertices of mesh
	* @param frags Fragments of mesh
	* @param textures Textures of mesh
	* @param materials Materials
	*/
	void addMesh(const std::string& mesh_name, const std::vector<TAGMesh::Vertex>& vertices, const std::vector<TAGMesh::Fragment>& frags, const std::vector<TAGMesh::Material>& materials);
	/**
	 * Delete a mesh.
	 * 
	 * @param mesh_name Name of mesh.
	 */
	void deleteMesh(const std::string& mesh_name);
private:
	void loadModel(const std::string& path);
	static ShaderObject shaderConverter(const Object& obj, const GLuint& split);
	const TAGTexLoader::Texture loadMaterialTexture(const std::string& tex_path, const TAGTexType& tex_type) const;
protected:
	std::string directory;
	TAGResourceManager::BufferAccess access;
	TAGTexLoader::Params tex_params;
	std::vector<std::string> mesh_draw_order;
	std::unordered_map<std::string, TAGMesh> meshes;
	std::unordered_map<std::string, TAGResourceManager::ObjectBuffer<Object, ShaderObject>> instance_buffers;
};

namespace ObjectMemberName {
	struct POSITION : TagStruct { using TYPE = glm::vec3; static constexpr std::size_t OFFSET = offsetof(TAGModel::Object, position); };
	struct ROTATION_AXIS : TagStruct { using TYPE = glm::vec3; static constexpr std::size_t OFFSET = offsetof(TAGModel::Object, rotation_axis); };
	struct ANGLE : TagStruct { using TYPE = float; static constexpr std::size_t OFFSET = offsetof(TAGModel::Object, angle); };
	struct SCALE : TagStruct { using TYPE = float; static constexpr std::size_t OFFSET = offsetof(TAGModel::Object, scale); };
};
