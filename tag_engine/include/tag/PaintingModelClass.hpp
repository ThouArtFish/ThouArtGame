#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "BaseStateClass.hpp"
#include "ModelClass.hpp"
#include "ShaderManagerClass.hpp"
#include "TextureLoaderClass.hpp"
#include "MeshClass.hpp"

/**
* Subclass of TAGModel for creating "billboard" models; 2D images that exist as painted planes within the game.
*/
class TAGPaintingModel : public TAGModel {
public:
	/**
	* @param paths The paths to each image to be used as models
	* @param tex_params The texture loading parameters for each image
	* @param material Material for each image
	*/
	TAGPaintingModel(const std::vector<std::string>& paths, const TAGTexLoader::Params& tex_params, const TAGMesh::Material& material = {});
	/**
	* @param Path to an image to be used as a model, can be left empty to add images later
	* @param tex_params The texture loading parameters for the image
	* @param material Material for the image
	*/
	TAGPaintingModel(const TAGTexLoader::Params& tex_params, const std::string& path = "", const TAGMesh::Material& material = {});
	/**
	* Add a new image model. This replaces the addMesh function from TAGModel.
	* 
	* @param path Path to an image to be used as a model
	* @param tex_params The texture loading parameters for the image
	* @param mat_mod Material modification values for the image
	*/
	void loadPainting(const std::string& path, const TAGTexLoader::Params& tex_params, const TAGMesh::Material& material = {});
	/**
	* Makes the Object reference face towards the point.
	* If lock_axis is true, the axis of the image model won't change after turning.
	* 
	* @param position Position to make painting face
	* @param obj Object reference to change facing direction
	* @param lock_axis Whether to stop the image from changing its vertical axis while turning or not.
	*/
	void faceDirec(const glm::vec3& position, Object& obj, const bool& lock_axis);
private:
	inline static constexpr std::array<glm::vec2, 4> tex_coords = { glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f) };
	inline static constexpr TAGMesh::Fragment frag_1 = { { 0, 1, 3 }, 0 };
	inline static constexpr TAGMesh::Fragment frag_2 = { { 1, 2, 3 }, 0 };
	inline static constexpr glm::vec3 defArm = { -1.0f, 0.0f, 0.0f };
	inline static constexpr glm::vec3 defNormal = { 0.0f, 0.0f, 1.0f };

	using TAGModel::addMesh;
};

