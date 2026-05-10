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
			TAGResourceManager::deleteBuffer<ProgramShader>(pair.second.ID);
		}
	}
}

unsigned int TAGShaderManager::loadShader(Source source) {
	if (source.is_path) {
		loadFromFile(source);
	}

	// Compile shaders
	unsigned int vertex, fragment;
	int success;
	char infoLog[512];
	static std::vector<GLchar*> source_ptr;

	vertex = TAGResourceManager::createBuffer<VertexShader>();
	source_ptr.push_back((GLchar*)source.vertex.c_str());
	glShaderSource(vertex, 1, source_ptr.data(), NULL);
	glCompileShader(vertex);
	source_ptr.pop_back();
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertex, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	};

	fragment = TAGResourceManager::createBuffer<FragmentShader>();
	source_ptr.push_back((GLchar*)source.fragment.c_str());
	glShaderSource(fragment, 1, source_ptr.data(), NULL);
	glCompileShader(fragment);
	source_ptr.pop_back();
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragment, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	};

	// shader Program
	const unsigned int ID = TAGResourceManager::createBuffer<ProgramShader>();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	TAGResourceManager::deleteBuffer<VertexShader>(vertex);
	TAGResourceManager::deleteBuffer<FragmentShader>(fragment);
	return ID;
}

void TAGShaderManager::loadFromFile(Source& source) {
	std::ifstream file;
	std::stringstream stream;

	// Read source code from shader files and except errors
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		file.open(TAGResourceManager::asset_path + source.vertex);
		stream << file.rdbuf();
		file.close();
		source.vertex = stream.str();

		file.open(TAGResourceManager::asset_path + source.fragment);
		stream << file.rdbuf();
		file.close();
		source.fragment = stream.str();
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}
}

void TAGShaderManager::addShader(const Source& source) {
	shaders.try_emplace(source.name, loadShader(source));
}

void TAGShaderManager::addShader(const std::vector<Source>& sources) {
	for (const Source& source : sources) {
		shaders.try_emplace(source.name, loadShader(source));
	}
}

void TAGShaderManager::deleteShader(const std::string& name) {
	TAGResourceManager::deleteBuffer<ProgramShader>(shaders.at(name).ID);
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
void TAGShaderManager::stopShader() const {
	glUseProgram(0);
}

std::vector<std::string> TAGShaderManager::getShaderNames() const {
	std::vector<std::string> names;
	names.reserve(shaders.size());
	for (const auto& pair : shaders) {
		names.push_back(pair.first);
	}
	return names;
}

auto TAGShaderManager::begin() const {
	return shaders.begin();
}

auto TAGShaderManager::end() const {
	return shaders.end();
}
