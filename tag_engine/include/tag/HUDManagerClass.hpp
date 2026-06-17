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

namespace QuadMemberName {
	struct TagStruct {};
	struct POSITION;
	struct DIMENSIONS;
	struct IMAGE_NAME;
	struct POSITION_FORMAT;
	struct DIMENSION_FORMAT;
	struct TEXEL_FORMAT;
	struct TEXEL_BOTTOM_LEFT;
	struct TEXEL_TOP_RIGHT;

	template<typename T> concept Concept = !std::same_as<T, TagStruct> && std::derived_from<T, TagStruct>;
};

class TAGHUDManager : public TAGBaseState::OpenGLContextChecker {
public:
	/**
	* How vectors represent positions on the screen or textures
	* RELATIVE - Values must be within 0 and 1 and represent the proportion of distance from the top left of the surface
	* PIXEL - Values represent the number of pixels (or texels) from the top left of the surface
	*/
	enum class DimensionFormat {
		RELATIVE,
		PIXEL
	};

	/**
	* Holds data about an image to be displayed on the screen
	*/
	struct Quad {
		glm::vec2 position;
		glm::vec2 dimensions;
		std::string image_name;
		unsigned int layer = 0;
		DimensionFormat position_format = DimensionFormat::RELATIVE;
		DimensionFormat dimension_format = DimensionFormat::RELATIVE;
		DimensionFormat texel_format = DimensionFormat::RELATIVE;
		glm::vec2 texel_bottom_left = { 0.0f, 0.0f };
		glm::vec2 texel_top_right = { 1.0f, 1.0f };
	};

	/**
	* Quad representation within buffer
	*/
	struct ShaderQuad {
		glm::vec4 quad_data;
		glm::vec4 texel_data;
		GLuint tex_index;
	};

	static inline GLuint base_attrib = 0;

	/**
	* @param paths Paths to each image to be loaded
	* @param access Frequency of quad changes
	* @param size Initial number of quad objects
	* @param params Texture loading parameters for each image
	*/
	TAGHUDManager(const std::vector<std::string>& paths, const TAGResourceManager::BufferAccess& access, const GLuint& size, const TAGTexLoader::Params& params = {});
	/**
	* @param params Texture loading parameters for singular path
	* @param access Frequency of quad changes
	* @param size Initial number of quad objects
	* @param path Path to an image
	*/
	TAGHUDManager(const TAGResourceManager::BufferAccess& access, const GLuint& size, const TAGTexLoader::Params& params = {}, const std::string& path = "");
	~TAGHUDManager();
	/**
	* Load an image from a path
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
	* Get identifiers of all images.
	*/
	std::vector<std::string> getImageNames() const;
	/**
	* Show or hide a layer.
	* 
	* @param layer Layer to change visibility.
	* @param state True to show, False to hide, or Toggle to switch.
	*/
	void setLayerVisibility(const GLuint& layer, const TAGEnum& state);
	/**
	* Adds quad in order.
	* Pushes to end of quad array if no index is passed.
	* A buffer update function must be used for changes to be reflected in the GPU buffer.
	*
	* @param quad New quad
	* @param index Index in light array
	*/
	void addQuad(const Quad& quad);
	/**
	* Sets a quad in internal array.
	* Layer ID will be changed to the layer ID of the quad originally at that position to maintain order.
	* 
	* @param quad New quad
	* @param index Index in array
	*/
	void setQuad(const Quad& quad, const GLuint& index);
	/**
	* Set an attribute of an existing quad struct, based on the offset of the attribute in the Quad struct
	* 
	* @param value Value to set
	* @param index Index in array
	*/
	template<QuadMemberName::Concept T> void setQuadMember(const T::TYPE& value, const GLuint& index);
	/**
	* Removes quad at index.
	* Pops last quad if no index is passed.
	* A buffer update function must be used for changes to be reflected in the GPU buffer.
	*
	* @param index Index in quad array
	*/
	Quad removeQuad(const int& index = -1);
	/**
	* Get quad at index.
	* Gets last quad if no index is passed.
	*
	* @param index Index in quad array
	*/
	const Quad& getQuad(const int& index = -1) const;
	/**
	* Get all quads.
	*/
	const std::vector<Quad>& getAllQuads() const;
	/**
	* Updates GPU side buffer with CPU side quad.
	*/
	void updateQuadBuffer();
	/**
	* Set a window width and height.
	* This is required if any Quads have position and dimension format set to PIXEL, as this is relative to screen dimensions.
	*/
	void setWindowDimensions(const int& width, const int& height);
	/**
	* Draw all visible quads
	* 
	* @param shader Shader used for drawing
	* @param texture_array_name Name of uniform in shader for array of texture IDs
	*/
	void drawAll(const TAGShaderManager::Shader& shader, const std::string& texture_array_name = TAGShaderManager::default_options.diffuse_tex_array);
private:
	/**
	* Holds data about a layer
	*/
	struct LayerData {
		GLuint id;
		GLuint start_index;
		GLuint count;
		bool is_hidden;
	};

	static inline GLuint VAO = 0;
	static inline GLuint VBO = 0;
	static inline GLuint EBO = 0;
	static inline constexpr GLuint MAX_TEXTURES = 16;

	glm::ivec2 screen_dimensions = { 1, 1 };
	std::vector<TAGTexLoader::Texture> images;
	TAGResourceManager::ObjectBuffer<Quad, ShaderQuad> quads;
	TAGResourceManager::ObjectBuffer<LayerData, OpenGLIndirectCommand> layers = { 10, commandConverter, TAGResourceManager::BufferAccess::DYNAMIC };
	std::vector<unsigned int> used_images;

	static void initMesh();
	static OpenGLIndirectCommand commandConverter(const LayerData& layer_data, const GLuint& split);
	ShaderQuad shaderConverter(const Quad& quad, const GLuint& split);
};

namespace QuadMemberName {
	struct POSITION : TagStruct { using TYPE = glm::vec2; static constexpr std::size_t OFFSET = offsetof(TAGHUDManager::Quad, position); };
	struct DIMENSIONS : TagStruct { using TYPE = glm::vec2; static constexpr std::size_t OFFSET = offsetof(TAGHUDManager::Quad, dimensions); };
	struct IMAGE_NAME : TagStruct { using TYPE = std::string; static constexpr std::size_t OFFSET = offsetof(TAGHUDManager::Quad, image_name); };
	struct POSITION_FORMAT : TagStruct { using TYPE = TAGHUDManager::DimensionFormat; static constexpr std::size_t OFFSET = offsetof(TAGHUDManager::Quad, position_format); };
	struct DIMENSION_FORMAT : TagStruct { using TYPE = TAGHUDManager::DimensionFormat; static constexpr std::size_t OFFSET = offsetof(TAGHUDManager::Quad, dimension_format); };
	struct TEXEL_FORMAT : TagStruct { using TYPE = TAGHUDManager::DimensionFormat; static constexpr std::size_t OFFSET = offsetof(TAGHUDManager::Quad, texel_format); };
	struct TEXEL_BOTTOM_LEFT : TagStruct { using TYPE = glm::vec2; static constexpr std::size_t OFFSET = offsetof(TAGHUDManager::Quad, texel_bottom_left); };
	struct TEXEL_TOP_RIGHT : TagStruct { using TYPE = glm::vec2; static constexpr std::size_t OFFSET = offsetof(TAGHUDManager::Quad, texel_top_right); };
};

#include "../../src/HUDManagerClass.inl"
