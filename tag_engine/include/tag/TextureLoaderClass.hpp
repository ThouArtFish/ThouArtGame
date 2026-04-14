#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>
#include "ResourceManagerClass.hpp"

/**
 * The type of a texture.
 */
enum class TAGTexType {
	SPEC_MAP,
	DIFFUSE_MAP,
	NORMAL_MAP,
	NONE_MAP
};

/**
 * Texture types
 */
enum class TAGTexParam {
	REPEAT_TEX = GL_REPEAT,
	CLAMP_TO_EDGE_TEX = GL_CLAMP_TO_EDGE,
	MIRROR_TEX = GL_MIRRORED_REPEAT,
	LINEAR_INTERP_PIX = GL_LINEAR_MIPMAP_LINEAR,
	NEAREST_PIX = GL_NEAREST_MIPMAP_NEAREST
};

/**
 * Loads images. Probably not needed to be user by the user explicitly.
 */
class TAGTexLoader {
	public:
		/**
		* Handles pointers to image data blocks
		*/
		struct ImageDataContainer {
			unsigned char* data;
			~ImageDataContainer();
		};

		/**
		 * Container for image raw data
		 */
		struct Info {
			ImageDataContainer data_container;
			int width;
			int height;
			int nr_channels;
		};
		/*
		* Container for info on image loading and texture setup
		*/
		struct Params {
			TAGTexParam wrap_type = TAGTexParam::REPEAT_TEX;
			TAGTexParam min_filter = TAGTexParam::LINEAR_INTERP_PIX;
			TAGTexParam mag_filter = TAGTexParam::NEAREST_PIX;
			bool srgb = false;
			bool flip = false;
		};

		/**
		* Texture container
		*/
		struct Texture {
			std::string name;
			unsigned int id;
			unsigned int width;
			unsigned int height;
			TAGTexType type = TAGTexType::NONE_MAP;
		};

		/*
		* Returns raw data of image file at path.
		* 
		* @param tex_path The path to the image.
		* @param flip Whether to flip the image on load or not.
		* @return Raw image data container.
		*/
		static Info loadRawImageData(const std::string& tex_path, const bool& flip);
		/*
		* Creates texture buffer from raw image data and texture parameters
		* 
		* @param info Raw image data container.
		* @param name Name of texture
		* @param params Settings for texture setup.
		* @return Texture struct containing info about texture handle buffer, such as ID.
		*/
		static Texture textureFromInfo(const Info& info, const std::string& name, const Params& params = {});
		/*
		* Creates texture buffer from image path and texture parameters
		*
		* @param tex_path The path to the image.
		* @param params Settings for texture setup.
		* @param name Name of texture, if blank then texture filename is used.
		* @return Texture struct containing info about texture handle buffer, such as ID.
		*/
		static Texture textureFromFile(const std::string& tex_path, const Params& params = {}, const std::string& name = "");
		/*
		* Creates texture buffer from directory path and texture parameters for a cubemap.
		* The images within the directory should be named "right", "left", "top", "bottom", "front" and "back"
		* for each side of the cubemap.
		*
		* @param folder_path Path to folder containing images.
		* @param params Settings for texture setup.
		* @return ID for OpenGL texture buffer.
		*/
		static unsigned int cubemapFromFile(const std::string& folder_path, const Params& params = {});
	private:
		static GLenum getTextureFormat(const int&, const bool&);
		static GLenum removeMipmapTag(const TAGTexParam& param);
};
