#include <TextureLoaderClass.hpp>

TAGTexLoader::ImageDataContainer::~ImageDataContainer() {
	stbi_image_free(data);
}

TAGTexLoader::Info TAGTexLoader::loadRawImageData(const std::string& tex_path, bool flip) {
	Info tex_info;
	stbi_set_flip_vertically_on_load(flip);
	tex_info.data_container.data = stbi_load(tex_path.c_str(), &tex_info.width, &tex_info.height, &tex_info.nr_channels, 0);

	if (!tex_info.data_container.data) std::cout << "Texture failed to load at path: " << tex_path << std::endl;

	return tex_info;
}

TAGTexLoader::Texture TAGTexLoader::textureFromInfo(const Info& tex_info, const std::string& name, const Params& params) {
	GLuint ID = TAGResourceManager::createBuffer<OpenGLObjectType::TextureBuffer>();
	glBindTexture(GL_TEXTURE_2D, ID);
	glTexImage2D(
		GL_TEXTURE_2D, 
		0, 
		getTextureInternalFormat(tex_info.nr_channels, params.srgb), 
		tex_info.width,
		tex_info.height, 
		0, 
		getTextureDataFormat(tex_info.nr_channels), 
		GL_UNSIGNED_BYTE, 
		tex_info.data_container.data
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (params.mipmap ? addMipmapTag(params.mag_filter) : (GLenum)params.mag_filter));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLenum)params.min_filter);
	if (params.mipmap) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	return { name, ID, (GLuint)tex_info.width, (GLuint)tex_info.height, TAGTexType::TWODIM };
}

TAGTexLoader::Texture TAGTexLoader::textureFromFile(const std::string& tex_path, const std::string& name, const Params& params) {
	return textureFromInfo(loadRawImageData(tex_path, params.flip), (name == "" ? static_cast<std::filesystem::path>(tex_path).stem().string() : name), params);
}

TAGTexLoader::Texture TAGTexLoader::emptyTexture(const std::string& name, GLuint width, GLuint height, GLuint nr_channels, const Params& params) {
	GLuint ID = TAGResourceManager::createBuffer<OpenGLObjectType::TextureBuffer>();
	glBindTexture(GL_TEXTURE_2D, ID);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		getTextureInternalFormat(nr_channels, params.srgb),
		width,
		height,
		0,
		getTextureDataFormat(nr_channels),
		GL_UNSIGNED_BYTE,
		nullptr
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (params.mipmap ? addMipmapTag(params.mag_filter) : (GLenum)params.mag_filter));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLenum)params.min_filter);
	if (!params.mipmap) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	return { name, ID, width, height, TAGTexType::TWODIM };
}

TAGTexLoader::Texture TAGTexLoader::cubemapFromMultipleFiles(const std::string& name, const std::vector<std::string>& tex_paths, const Params& params) {
	if (tex_paths.size() < 6) return { .id = 0 };

	GLuint ID = TAGResourceManager::createBuffer<OpenGLObjectType::TextureBuffer>();
	glm::uvec2 dim;
	glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
	for (size_t i = 0; i < 6; i++) {
		Info tex_info = loadRawImageData(tex_paths.at(i), params.flip);

		if (i == 0) dim = { tex_info.width, tex_info.height };

		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0,
			getTextureInternalFormat(tex_info.nr_channels, params.srgb),
			tex_info.width, 
			tex_info.height, 
			0, 
			getTextureDataFormat(tex_info.nr_channels),
			GL_UNSIGNED_BYTE, 
			tex_info.data_container.data
		);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (params.mipmap ? addMipmapTag(params.mag_filter) : (GLenum)params.mag_filter));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, (GLenum)params.mag_filter);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, (GLenum)params.wrap_type);
	if (params.mipmap) {
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	}
	else {
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
	}
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	return { name, ID, dim.x, dim.x, TAGTexType::CUBEMAP };
}

TAGTexLoader::Texture TAGTexLoader::emptyCubemap(const std::string& name, GLuint face_size, GLuint nr_channels, const Params& params) {
	GLuint ID = TAGResourceManager::createBuffer<OpenGLObjectType::TextureBuffer>();
	glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
	for (size_t i = 0; i < 6; i++) {
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0,
			getTextureInternalFormat(nr_channels, params.srgb),
			face_size,
			face_size,
			0,
			getTextureDataFormat(nr_channels),
			GL_UNSIGNED_BYTE,
			nullptr
		);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (params.mipmap ? addMipmapTag(params.mag_filter) : (GLenum)params.mag_filter));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, (GLenum)params.mag_filter);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, (GLenum)params.wrap_type);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, (GLenum)params.wrap_type);
	if (!params.mipmap) glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	return { name, ID, face_size, face_size, TAGTexType::CUBEMAP };
}

void TAGTexLoader::generateMipmaps(const Texture& tex) {
	const GLenum type = (GLenum)tex.type;
	glBindTexture(type, tex.id);
	glTexParameteri(type, GL_TEXTURE_MAX_LEVEL, 1000);
	glGenerateTextureMipmap(type);
	glBindTexture(type, 0);
}

GLenum TAGTexLoader::getTextureInternalFormat(int nr_channels, bool srgb) {
	switch (nr_channels) {
	case 1:
		return GL_R8;
	case 2:
		return GL_RG8;
	case 3:
		return (srgb ? GL_SRGB8 : GL_RGB8);
	default: // case 4
		return (srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8);
	}
}

GLenum TAGTexLoader::getTextureDataFormat(int nr_channels) {
	switch (nr_channels) {
	case 1:
		return GL_RED;
	case 2:
		return GL_RG;
	case 3:
		return GL_RGB;
	default: // case 4
		return GL_RGBA;
	}
}

GLenum TAGTexLoader::addMipmapTag(TAGTexParam param) {
	return (param == TAGTexParam::LINEAR_INTERP_PIX ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
}
