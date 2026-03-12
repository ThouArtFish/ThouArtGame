#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tinyobjloader/tiny_obj_loader.h>
#include "ResourceManagerClass.hpp"
#include "BaseStateClass.hpp"
#include "TextureLoaderClass.hpp"
#include "ShaderManagerClass.hpp"
#include "MeshClass.hpp"

/**
 * Stores the contents of the "model" at the path passed to the constructor, which can consist of many meshes.
 * Handles in-game instances of meshes through the Object struct and its associated functions.
 * Many functions have the feature to not pass a mesh name, which means the function is handling an instance of
 * stored meshes, instead of just one mesh, although it could be one mesh.
 */
class TAGModel {
public:
	/**
	 * Container for various info describing an instance of a mesh in-game.
	 */
	struct Object {
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 rotation_axis = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 velocity = glm::vec3(0.0f);
		float angle = 0.0f;
		float angular_velocity = 0.0f;
		float scale = 1.0f;
	};
	/**
	* Contains the transformation applied to a mesh for an instance
	*/
	struct ObjectTrans {
		glm::mat4 model;
		glm::mat3 normal;
		ObjectTrans(const Object& obj);
	};

	/**
	 * Path to the model file, and parameters for texture loading.
	 * Path can be empty, so TAGMesh can be added later.
	 *  
	 * @param params Parameters for texture loading.
	 * @param path Path to model file.
	 */
	TAGModel(const TAGTexLoader::Params& params, const std::string& path = "");
	TAGModel(const TAGModel&) = delete;
	TAGModel& operator=(const TAGModel&) = delete;

	/**
	 * Draws all instances of the specified mesh, or all instances which represent all meshes if no mesh name is passed.
	 * 
	 * @param shader Shader to draw with.
	 * @param cull_face Whether to cull inside faces, generally true for 3D models.
	 * @param mesh_name Name of mesh to draw all instances of, can be left as default.
	 */
	void drawAll(const TAGShaderManager::Shader& shader, const bool& cull_face, const std::string& mesh_name = "");
	/**
	 * Draws one instance of a specified mesh, or one instance of all meshes if no mesh name is passed.
	 * 
	 * @param shader Shader to draw with.
	 * @param obj Instance of mesh to draw.
	 * @param cull_face Whether to cull inside faces, generally true for 3D models.
	 * @param mesh_name Name of mesh to draw all instances of, can be left as default.
	 */
	void drawOne(const TAGShaderManager::Shader& shader, const Object& obj, const bool& cull_face, const std::string& mesh_name = "");
	/**
	 * Sets the size of the instance buffer for the specified mesh.
	 * This is not a hard cap, the buffer is resized if more instances are added than size permits.
	 * 
	 * @param size Maximum size of instance buffer.
	 * @param mesh_name Name of mesh.
	 */
	void setMaxInstanceCount(const unsigned int& size, const std::string& mesh_name = "");
	/**
	 * Get the instances of a specific mesh, or the instances that represent every mesh if no mesh name is passed.
	 * 
	 * @param mesh_name Name of mesh.
	 */
	const std::vector<Object>& getInstances(const std::string& mesh_name = "") const;
	/**
	* Get the instances of a specific mesh, or instances that represent every mesh if no mesh name is passed.
	* 
	* @param mesh_name Name of mesh.
	*/
	std::vector<Object>& changeInstances(const std::string& mesh_name = "");
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
	* @param vertices Vertices of mesh
	* @param frags Fragments of mesh
	* @param textures Textures of mesh
	* @param material_mod Material modifiers
	*/
	void addMesh(const std::string& mesh_name, const std::vector<TAGMesh::Vertex>& vertices, const std::vector<TAGMesh::Fragment>& frags, const std::vector<TAGMesh::Texture>& textures, const TAGMesh::MaterialMod& material_mod);
	/**
	 * Delete a mesh.
	 * 
	 * @param mesh_name Name of mesh.
	 */
	void deleteMesh(const std::string& mesh_name);
private:
	void loadModel(const std::string& path);
	const TAGMesh::Texture loadMaterialTexture(const std::string& tex_path, const TAGTexType& tex_type) const;
	const TAGMesh::Texture loadMaterialTexture(const glm::vec3& vec, const TAGTexType& tex_type) const;
protected:
	struct InstanceDrawBuffer {
		bool was_updated = true;
		unsigned int vbo = 0;
		unsigned int max_instances = 0;

		InstanceDrawBuffer();
		~InstanceDrawBuffer();
	};

	std::string directory;
	TAGTexLoader::Params tex_params;
	std::vector<std::string> mesh_draw_order;
	std::unordered_map<std::string, TAGMesh> meshes;
	std::unordered_map<std::string, InstanceDrawBuffer> instance_buffers;
	std::unordered_map<std::string, std::vector<Object>> instances;
};