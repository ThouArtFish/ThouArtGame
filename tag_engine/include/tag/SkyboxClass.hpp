#pragma once

#include <string>
#include <array>
#include <glad/glad.h>
#include "BaseStateClass.hpp"
#include "ResourceManagerClass.hpp"
#include "ShaderManagerClass.hpp"
#include "TextureLoaderClass.hpp"

/**
* Handles an instane of a skybox
*/
class TAGSkybox {
	public:
		bool delete_on_death = true;

		/**
		* Pass a path to the directory containing all skybox images.
		* Params decide the loading parameters for each skybox texture.
		*/
		TAGSkybox(const std::string& directory, const TAGTexLoader::Params& params);
		~TAGSkybox();
		/**
		* Draws the skybox.
		* 
		* @param shader The shader to draw the skybox with
		*/
		void draw(const TAGShaderManager::Shader& shader) const;
	private:
		unsigned int VBO, EBO, VAO, cubemap_ID;
};


