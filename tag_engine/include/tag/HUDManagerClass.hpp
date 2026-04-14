#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <array>
#include "TextureLoaderClass.hpp"
#include "ResourceManagerClass.hpp"
#include "ShaderManagerClass.hpp"
#include "UtilClass.hpp"

/**
* How vectors represent positions on the screen or textures
* RELATIVE - Values must be within 0 and 1 and represent the proportion of distance from the top left of the surface
* PIXEL - Values represent the number of pixels (or texels) from the top left of the surface
*/
enum class TAGHUDQuadFormat {
	RELATIVE,
	PIXEL
};

class TAGHUDManager {
public:
	/**
	* Holds data about an image to be displayed on the screen
	*/
	struct Quad {
		glm::vec2 position;
		glm::vec2 dimensions;
		std::string image_name;
		unsigned int layer;
		TAGHUDQuadFormat position_format = TAGHUDQuadFormat::RELATIVE;
		TAGHUDQuadFormat dimension_format = TAGHUDQuadFormat::RELATIVE;
		TAGHUDQuadFormat texel_format = TAGHUDQuadFormat::RELATIVE;
		glm::vec2 texel_top_left = { 0.0f, 0.0f };
		glm::vec2 texel_bottom_right = { 1.0f, 1.0f };
	};

	/**
	* Quad representation within buffer
	*/
	struct BufferQuad {
		glm::vec2 trans;
		glm::vec2 scale;
		glm::vec2 texel_trans;
		glm::vec2 texel_scale;
		unsigned int tex_index;
	};

	/**
	* Holds data about a layer
	*/
	struct LayerData {
		unsigned int id;
		bool is_hidden;
	};

	bool delete_on_death = true;
	static inline unsigned int base_attrib = 0;

	/**
	* @param paths Paths to each image to be loaded
	* @param params Texture loading parameters for each image
	*/
	TAGHUDManager(const std::vector<std::string>& paths, const TAGTexLoader::Params& params = {});
	/**
	* @param params Texture loading parameters for singular path
	* @param path Path to an image
	*/
	TAGHUDManager(const TAGTexLoader::Params& params = {}, const std::string& path = "");
	~TAGHUDManager();
	/**
	* Load an image from a path, identifier will be filename
	* 
	* @param path Path to image
	* @param params Texture loading parameters for image
	* @param name Name of texture. If not provided, filename is used
	*/
	void loadImage(const std::string& path, const TAGTexLoader::Params& params = {}, const std::string& name = "");
	/**
	* Add an image with an already loaded texture
	* 
	* @param texture Texture struct
	*/
	void addImage(const TAGTexLoader::Texture& texture);
	/**
	* Delete image with identifier
	* 
	* @param name Identifier of image
	* @param global_delete Delete texture buffer as well, invalidating the texture ID for any other objects using it
	*/
	void deleteImage(const std::string& name, const bool& global_delete = true);
	/**
	* Make changes to current array of Quads
	*/
	std::vector<Quad>& changeQuads();
	/**
	* View current array of Quads
	*/
	const std::vector<Quad>& getQuads() const;
	/**
	* Show or hide a layer
	* 
	* @param layer Layer to change visibility
	* @param state True to show, False to hide, or Toggle to switch
	*/
	void setLayerVisibility(const unsigned int& layer, const TAGEnum& state);
	/**
	* View current layers in order of lowest to highest
	*/
	const std::vector<LayerData>& getLayers() const;
	/**
	* Set a window width and height.
	* This is required if any Quads have position and dimension format set to PIXEL, as this is relative to screen dimensions.
	*/
	void setWindowDimensions(const int& width, const int& height);
	/**
	* Get identifiers of all images;
	*/
	std::vector<std::string> getImageNames() const;
	/**
	* Update OpenGL buffer holding data about image quads.
	* Required before drawing if the same reference to the quad vector is changed after drawing once.
	*/
	void updateQuadBuffer();
	/**
	* Draw all visible quads
	* 
	* @param shader Shader used for drawing
	*/
	void drawAll(const TAGShaderManager::Shader& shader);
private:
	static inline unsigned int VAO = 0;
	static inline unsigned int VBO = 0;
	static inline unsigned int EBO = 0;
	static inline constexpr unsigned int NUM_FENCES = 3;
	static inline constexpr unsigned int MAX_TEXTURES = 16;

	glm::ivec2 screen_dimensions = { 1, 1 };
	unsigned int buffer_quad_count = 0;
	unsigned int current_fence = 0;
	unsigned int max_quads = 100;
	unsigned int region_size = max_quads * sizeof(BufferQuad);
	unsigned int total_size = region_size * NUM_FENCES;
	unsigned int quad_buffer = 0;
	BufferQuad* quad_buffer_ptr = nullptr;
	bool quads_changed = true;
	std::array<GLsync, NUM_FENCES> fences = {};
	std::vector<TAGTexLoader::Texture> images;
	std::vector<LayerData> layers;
	std::vector<unsigned int> used_images;
	std::vector<Quad> quads;

	void initMesh();
};