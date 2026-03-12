#include <TextureLoaderClass.hpp>

TAGTexLoader::Info TAGTexLoader::loadRawImageData(const std::string& tex_path, const bool& flip) {
	Info tex_info;
	stbi_set_flip_vertically_on_load(flip);
	tex_info.data = stbi_load(tex_path.c_str(), &tex_info.width, &tex_info.height, &tex_info.nr_channels, 0);
	if (!tex_info.data) {
		std::cout << "Texture failed to load at path: " << tex_path << std::endl;
	}
	return tex_info;
}

GLenum TAGTexLoader::getTextureFormat(const int& nr_channels, const bool& srgb) {
	switch (nr_channels) {
	case 1:
		return GL_RED;
	case 3:
		return (srgb ? GL_SRGB8 : GL_RGB);
	default:
		return (srgb ? GL_SRGB8_ALPHA8 : GL_RGBA);
	}
}

GLenum TAGTexLoader::getMagTexParam(const TAGTexParam& param) {
	return (param == TAGTexParam::LINEAR_INTERP_PIX ? GL_LINEAR : GL_NEAREST);
}

unsigned int TAGTexLoader::textureFromInfo(const Info& tex_info, const Params& params) {
	unsigned int ID = TAGResourceManager::createBuffer<OpenGLTexture>();
	const GLenum format = getTextureFormat(tex_info.nr_channels, params.srgb);
	glBindTexture(GL_TEXTURE_2D, ID);
	glTexImage2D(GL_TEXTURE_2D, 0, format, tex_info.width, tex_info.height, 0, format - (params.srgb && format != GL_RED ? 29499 : 0), GL_UNSIGNED_BYTE, tex_info.data);
	stbi_image_free(tex_info.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLenum)params.min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, getMagTexParam(params.mag_filter));
	glBindTexture(GL_TEXTURE_2D, 0);
	return ID;
}

unsigned int TAGTexLoader::textureFromFile(const std::string& path, const Params& params) {
	return textureFromInfo(loadRawImageData(path, params.flip), params);
}

unsigned int TAGTexLoader::textureFromColour(const glm::vec3& colour) {
	unsigned int ID = TAGResourceManager::createBuffer<OpenGLTexture>();
	glBindTexture(GL_TEXTURE_2D, ID);
	const char rgb[3] = {(char)round(colour.r * 255), (char)round(colour.g * 255), (char)round(colour.b * 255)};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	return ID;
}

unsigned int TAGTexLoader::cubemapFromFile(const std::string& folder_path, const Params& params) {
	static const std::vector<std::string> filenames = {
		"right.jpg",
		"left.jpg",
		"top.jpg",
		"bottom.jpg",
		"front.jpg",
		"back.jpg"
	};
	unsigned int ID = TAGResourceManager::createBuffer<OpenGLTexture>();
	glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
	for (unsigned int i = 0; i < filenames.size(); i++)
	{
		const Info tex_info = loadRawImageData(folder_path + filenames[i], params.flip);
		const GLenum format = getTextureFormat(tex_info.nr_channels, params.srgb);
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0,
			format, tex_info.width, tex_info.height, 0, format - (params.srgb && format != GL_RED ? 29499 : 0), GL_UNSIGNED_BYTE, tex_info.data
		);
		stbi_image_free(tex_info.data);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (GLenum)params.min_filter);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, getMagTexParam(params.mag_filter));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, (GLenum)params.wrap_type);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	return ID;
}