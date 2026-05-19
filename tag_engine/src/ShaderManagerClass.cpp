#include <ShaderManagerClass.hpp>

TAGShaderManager::TAGShaderManager(const Source& source) {
	addShader(source);
}

TAGShaderManager::TAGShaderManager(const std::vector<Source>& sources) {
	addShader(sources);
}

TAGShaderManager::~TAGShaderManager() {
	if (delete_on_death) {
		for (const auto& pair : shaders) {
			TAGResourceManager::deleteBuffer<TAGResourceManager::ProgramShader>(pair.second.ID);
		}
	}
}

TAGShaderManager::Shader TAGShaderManager::loadShader(const Source& source) {
	std::string vertex_code, fragment_code;
	if (source.is_path) {
		getSourceCodeFromFile(source, vertex_code, fragment_code);
	}

	// Compile shader program stages
	unsigned int vertex, fragment;
	int success;
	char infoLog[512];
	static std::vector<GLchar*> source_ptr;

	vertex = TAGResourceManager::createBuffer<TAGResourceManager::VertexShader>();
	source_ptr.push_back((GLchar*)(source.is_path || source.shader_type != ShaderType::CUSTOM_DRAW ? vertex_code : source.vertex).c_str());
	glShaderSource(vertex, 1, source_ptr.data(), NULL);
	glCompileShader(vertex);
	source_ptr.pop_back();
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	};

	fragment = TAGResourceManager::createBuffer<TAGResourceManager::FragmentShader>();
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
	const unsigned int ID = TAGResourceManager::createBuffer<TAGResourceManager::ProgramShader>();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	TAGResourceManager::deleteBuffer<TAGResourceManager::VertexShader>(vertex);
	TAGResourceManager::deleteBuffer<TAGResourceManager::FragmentShader>(fragment);

	// Get location and names of all uniforms
	Shader shader = { .ID = ID };

	GLint uniform_count;
	glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &uniform_count);
	GLint max_uniform_name_length;
	glGetProgramiv(ID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_uniform_name_length);

	std::vector<GLchar> name(max_uniform_name_length);

	for (GLint i = 0; i < uniform_count; i++) {
		GLsizei length;
		GLint size;
		GLenum type;

		glGetActiveUniform(ID, i, max_uniform_name_length, &length, &size, &type, name.data());
		GLint location = glGetUniformLocation(ID, name.data());

		auto bracket_loc = std::find(name.begin(), name.end(), '[');
		if (bracket_loc != name.end()) {
			length -= 3;
		}
		shader.uniform_locations[std::string(name.data(), length)] = location;
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

void TAGShaderManager::addShader(const Source& source) {
	shaders.insert_or_assign(source.name, loadShader(source));
}

void TAGShaderManager::addShader(const std::vector<Source>& sources) {
	for (const Source& source : sources) {
		shaders.insert_or_assign(source.name, loadShader(source));
	}
}

void TAGShaderManager::deleteShader(const std::string& name) {
	TAGResourceManager::deleteBuffer<TAGResourceManager::ProgramShader>(shaders.at(name).ID);
	shaders.erase(name);
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
