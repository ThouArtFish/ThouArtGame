#include <TextureLoaderClass.hpp>

TAGTexLoader::ImageDataContainer::~ImageDataContainer() {
	stbi_image_free(data);
}

TAGTexLoader::Info TAGTexLoader::loadRawImageData(const std::string& tex_path, const bool& flip) {
	Info tex_info;
	stbi_set_flip_vertically_on_load(flip);
	tex_info.data_container.data = stbi_load(tex_path.c_str(), &tex_info.width, &tex_info.height, &tex_info.nr_channels, 0);
	if (!tex_info.data_container.data) {
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

GLenum TAGTexLoader::removeMipmapTag(const TAGTexParam& param) {
	return (param == TAGTexParam::LINEAR_INTERP_PIX ? GL_LINEAR : GL_NEAREST);
}

TAGTexLoader::Texture TAGTexLoader::textureFromInfo(const Info& tex_info, const std::string& name, const Params& params) {
	unsigned int ID = TAGResourceManager::createBuffer<OpenGLTexture>();
	const GLenum format = getTextureFormat(tex_info.nr_channels, params.srgb);
	glBindTexture(GL_TEXTURE_2D, ID);
	glTexImage2D(GL_TEXTURE_2D, 0, format, tex_info.width, tex_info.height, 0, format - (params.srgb && format != GL_RED ? 29499 : 0), GL_UNSIGNED_BYTE, tex_info.data_container.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLenum)params.min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, removeMipmapTag(params.mag_filter));
	glBindTexture(GL_TEXTURE_2D, 0);
	return { name, ID, (unsigned int)tex_info.width, (unsigned int)tex_info.height };
}

TAGTexLoader::Texture TAGTexLoader::textureFromFile(const std::string& tex_path, const Params& params, const std::string& name) {
	return textureFromInfo(loadRawImageData(tex_path, params.flip), (name == "" ? static_cast<std::filesystem::path>(tex_path).stem().string() : name), params);
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
	for (size_t i = 0; i < 6; i++)
	{
		const Info tex_info = loadRawImageData(folder_path + filenames[i], params.flip);
		const GLenum format = getTextureFormat(tex_info.nr_channels, params.srgb);
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + (unsigned int)i,
			0,
			format, 
			tex_info.width, 
			tex_info.height, 
			0, 
			format - (params.srgb && format != GL_RED ? 29499 : 0), 
			GL_UNSIGNED_BYTE, 
			tex_info.data_container.data
		);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, removeMipmapTag(params.min_filter));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, removeMipmapTag(params.mag_filter));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, (GLenum)params.wrap_type);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	return ID;
}