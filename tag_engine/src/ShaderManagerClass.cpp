#include <ShaderManagerClass.hpp>

TAGShaderManager::TAGShaderManager(const Source& source) {
	addShader(source);
}

TAGShaderManager::TAGShaderManager(const std::vector<Source>& sources) {
	addShader(sources);
}

TAGShaderManager::~TAGShaderManager() {
	for (const auto& pair : shaders) {
		TAGResourceManager::deleteBuffer<OpenGLObjectType::ShaderProgram>(pair.second.ID);
	}
}

TAGShaderManager::Shader TAGShaderManager::loadShader(const Source& source) {
	std::string vertex_code, fragment_code;
	if (source.shader_type != ShaderType::CUSTOM_DRAW) {
		getSourceCodeFromDefault(source, vertex_code, fragment_code);
	} else if (source.is_path) {
		getSourceCodeFromFile(source, vertex_code, fragment_code);
	}

	// Compile shader program stages
	GLuint vertex, fragment;
	int success;
	char infoLog[512];
	static std::vector<GLchar*> source_ptr;

	vertex = TAGResourceManager::createBuffer<OpenGLObjectType::VertexShader>();
	source_ptr.push_back((GLchar*)(source.is_path || source.shader_type != ShaderType::CUSTOM_DRAW ? vertex_code : source.vertex).c_str());
	glShaderSource(vertex, 1, source_ptr.data(), NULL);
	glCompileShader(vertex);
	source_ptr.pop_back();
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	};

	fragment = TAGResourceManager::createBuffer<OpenGLObjectType::FragmentShader>();
	source_ptr.push_back((GLchar*)(source.is_path || source.shader_type != ShaderType::CUSTOM_DRAW ? fragment_code : source.fragment).c_str());
	glShaderSource(fragment, 1, source_ptr.data(), NULL);
	glCompileShader(fragment);
	source_ptr.pop_back();
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	};

	// Compile final shader program
	const GLuint ID = TAGResourceManager::createBuffer<OpenGLObjectType::ShaderProgram>();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	TAGResourceManager::deleteBuffer<OpenGLObjectType::VertexShader>(vertex);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::FragmentShader>(fragment);

	Shader shader = { .ID = ID };

	// Get location and names of all uniforms
	GLint count;

	constexpr static GLsizei data_size = 100;
	std::array<GLint, data_size> data = {};
	using char_it = std::array<GLchar, data_size>::iterator;

	glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &count);
	for (GLint i = 0; i < count; i++) {
		GLsizei length;
		GLint size;
		GLenum type;

		GLchar* const chars = (GLchar*)data.data();

		glGetActiveUniform(ID, i, (GLsizei) data.max_size(), &length, &size, &type, chars);

		GLint location = glGetUniformLocation(ID, chars);

		if (std::find(chars, chars + length, '[') != chars + length) length -= 3;

		shader.uniform_data[std::string(chars, length)] = { location, type };
	}

	// Get attribute data
	glGetProgramiv(ID, GL_ACTIVE_ATTRIBUTES, &count);

	for (GLint i = 0; i < count; i++) {
		GLsizei length;
		GLint size;
		GLenum type;

		glGetActiveAttrib(ID, i, (GLsizei) data.max_size(), &length, &size, &type, (GLchar*)data.data());

		GLint location = glGetAttribLocation(ID, (GLchar*)data.data());

		shader.attribute_data[location] = { std::string((GLchar*)data.data(), length), type };
	}

	// Get data for each program interface
	constexpr std::array<GLenum, 2> props = { GL_BUFFER_BINDING, GL_NUM_ACTIVE_VARIABLES };
	for (const TAGResourceManager::ShaderBufferInterfaceType& type : TAGResourceManager::buffer_interface_types) {
		glGetProgramInterfaceiv(ID, (GLenum)type, GL_ACTIVE_RESOURCES, &count);
		for (GLint i = 0; i < count; i++) {
			GLsizei length;
			glGetProgramResourceiv(ID, (GLenum) type, i, (GLsizei) props.max_size(), props.data(), sizeof(data), &length, data.data());
			shader.buffer_locations[(GLuint) type].push_back(data[0]);
		}
	}

	return shader;
}

void TAGShaderManager::getSourceCodeFromFile(const Source& source, std::string& vertex_code, std::string& fragment_code) {
	std::ifstream file;
	std::stringstream stream;

	// Read source code from shader files and except errors
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		file.open(TAGResourceManager::asset_path + source.vertex);
		stream << file.rdbuf();
		file.close();
		vertex_code = stream.str();

		file.open(TAGResourceManager::asset_path + source.fragment);
		stream << file.rdbuf();
		file.close();
		fragment_code = stream.str();
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}
}

void TAGShaderManager::getSourceCodeFromDefault(const Source& source, std::string& vertex_code, std::string& fragment_code) {
	vertex_code = fragment_code = shader_version.substr();
	switch (source.shader_type) {
	case ShaderType::SKYBOX_DRAW:
		vertex_code += default_source[2].substr();
		fragment_code += default_source[4].substr();
		break;
	case ShaderType::UNINSTANCED_BASIC_DRAW:
		vertex_code += default_source[0].substr();
		fragment_code += default_source[5].substr();
		break;
	case ShaderType::INSTANCED_BASIC_DRAW:
		vertex_code += default_source[1].substr();
		fragment_code += default_source[5].substr();
		break;
	case ShaderType::HUD_DRAW:
		vertex_code += default_source[3].substr();
		fragment_code += default_source[6].substr();
		break;
	case ShaderType::UNINSTANCED_MODEL_DRAW:
		vertex_code += default_source[0].substr();
		fragment_code += default_source[7].substr();
		break;
	default: // INSTANCED_MODEL_DRAW
		vertex_code += default_source[1].substr();
		fragment_code += default_source[7].substr();
	}
}

void TAGShaderManager::addShader(const Source& source) {
	shaders.insert_or_assign(source.name, loadShader(source));
}

void TAGShaderManager::addShader(const std::vector<Source>& sources) {
	for (const Source& source : sources) {
		shaders.insert_or_assign(source.name, loadShader(source));
	}
}

void TAGShaderManager::deleteShader(const std::string& name) {
	if (shaders.erase(name) > 0) TAGResourceManager::deleteBuffer<OpenGLObjectType::ShaderProgram>(shaders.at(name).ID);
}

void TAGShaderManager::deleteShader(const std::vector<std::string>& names) {
	for (const std::string& name : names) {
		deleteShader(name);
	}
}

const TAGShaderManager::Shader& TAGShaderManager::useShader(const std::string& name) const {
	const Shader& shader = shaders.at(name);
	glUseProgram(shader.ID);
	return shader;
}

std::vector<std::string> TAGShaderManager::getShaderNames() const {
	std::vector<std::string> names;
	names.reserve(shaders.size());
	for (const auto& pair : shaders) {
		names.push_back(pair.first);
	}
	return names;
}

void TAGShaderManager::stopShader() {
	glUseProgram(0);
}
