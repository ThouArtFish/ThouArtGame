#pragma once

#include <string>
#include <array>
#include <glad/glad.h>
#include "BaseStateClass.hpp"
#include "ResourceManagerClass.hpp"
#include "ShaderManagerClass.hpp"
#include "TextureLoaderClass.hpp"

/**
* Handles an instance of a skybox
*/
class TAGSkybox : public TAGBaseState::OpenGLContextChecker {
	public:
		/**
		* Pass a path to single image, or multiple paths for each side of the skybox cubemap.
		* Params decide the loading parameters for each skybox texture.
		* 
		* @param path(s) Path to an image file
		* @param params Texture loading parameters
		*/
		TAGSkybox(const std::vector<std::string>& paths, const TAGTexLoader::Params& params = {});
		TAGSkybox(const std::string& path, const TAGTexLoader::Params& params = {});
		~TAGSkybox();

		/**
		* Draws the skybox.
		* 
		* @param shader The shader to draw the skybox with
		* @param cubemap_name Name of cubemap shader uniform
		*/
		void draw(const TAGShaderManager::Shader& shader, const std::string& cubemap_name = TAGShaderManager::default_options.cubemap) const;
	private:
		static void generateCube();
	
		static inline GLuint VBO = 0, EBO = 0, VAO = 0, cubemap_ID = 0;
};


