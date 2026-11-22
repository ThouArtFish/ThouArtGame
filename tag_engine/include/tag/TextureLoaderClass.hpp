#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <stb/stb_image.h>
#include "ResourceManagerClass.hpp"
#include "BaseStateClass.hpp"

/**
 * The type of a texture.
 */
enum class TAGTexType {
	TWODIM = GL_TEXTURE_2D,
	CUBEMAP = GL_TEXTURE_CUBE_MAP
};

/**
 * Texture types
 */
enum class TAGTexParam {
	REPEAT_TEX = GL_REPEAT,
	CLAMP_TO_EDGE_TEX = GL_CLAMP_TO_EDGE,
	MIRROR_TEX = GL_MIRRORED_REPEAT,
	LINEAR_INTERP_PIX = GL_LINEAR,
	NEAREST_PIX = GL_NEAREST
};

/**
 * Loads images from paths into Texture objects.
 */
class TAGTexLoader : public TAGBaseState::OpenGLContextChecker {
	public:
		/**
		* Handles pointers to image data blocks
		*/
		struct ImageDataContainer {
			unsigned char* data = nullptr;
			~ImageDataContainer();
		};

		/**
		 * Container for image raw data
		 */
		struct Info {
			ImageDataContainer data_container;
			int width = 1024, height = 1024, nr_channels = 3;
		};

		/*
		* Container for info on image loading and texture setup
		*/
		struct Params {
			TAGTexParam wrap_type = TAGTexParam::REPEAT_TEX;
			TAGTexParam min_filter = TAGTexParam::NEAREST_PIX;
			TAGTexParam mag_filter = TAGTexParam::NEAREST_PIX;
			bool srgb = false, flip = false, mipmap = true, internal_int = false;
		};

		/**
		* Texture container
		*/
		struct Texture {
			std::string name;
			GLuint id, width, height;
			TAGTexType type;
		};

		/*
		* Returns raw data of image file at path.
		* 
		* @param tex_path The path to the image.
		* @param flip Whether to flip the image on load or not.
		* @return Raw image data container.
		*/
		static Info loadRawImageData(const std::string& tex_path, bool flip);
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
		* @param name Name of texture, if blank then texture filename is used.
		* @param params Settings for texture setup.
		* @return Texture struct containing info about texture handle buffer, such as ID.
		*/
		static Texture textureFromFile(const std::string& tex_path, const std::string& name = "", const Params& params = {});
		/*
		* Creates an empty 2D texture.
		* 
		* @param name Name of texture.
		* @param width Width of texture.
		* @param height Height of texture.
		* @param nr_channels Number of colour channels for each pixel.
		* @param params Settings for texture setup.
		* @return Texture struct containing info about texture handle buffer, such as ID.
		*/
		static Texture emptyTexture(const std::string& name, GLuint width, GLuint height, GLuint nr_channels, const Params& params = {});
		/*
		* Creates a cubemap texture from the first 6 image paths in tex_paths, for each side of the cubemap.
		* Uses last path for remaining sides of cubemap if size < 6.
		* Returns invalid texture if tex_paths is empty.
		*
		* @param name Name of texture.
		* @param tex_paths Paths to image files.
		* @param params Settings for texture setup.
		* @return Texture struct containing info about texture handle buffer, such as ID.
		*/
		static Texture cubemapFromMultipleFiles(const std::string& name, const std::vector<std::string>& tex_paths, const Params& params = {});
		/*
		* Creates an empty cubemap texture.
		* 
		* @param name Name of texture.
		* @param face_size Length of each square face in the cubemap.
		* @param nr_channels Number of colour channels for each pixel.
		* @param params Settings for texture setup.
		* @return Texture struct containing info about texture handle buffer, such as ID.
		*/
		static Texture emptyCubemap(const std::string& name, GLuint face_size, GLuint nr_channels, const Params& params = {});
		/**
		* Generate mipmaps for a texture.
		* 
		* @param tex struct containing info about texture handle buffer, such as ID.
		*/
		static void generateMipmaps(const Texture& tex);
	private:
		static GLenum getTextureInternalFormat(int nr_channels, bool srgb);
		static GLenum getTextureDataFormat(int nr_channels);
		static GLenum addMipmapTag(TAGTexParam param);
};
