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
			TAGResourceManager::deleteBuffer<OpenGLProgram>(pair.second.ID);
		}
	}
}

unsigned int TAGShaderManager::loadShader(const std::string& vertexPath, const std::string& fragmentPath) {
	std::string vertexCode;
	std::string fragmentCode;
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;

	// Read source code from shader files and except errors
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		// open files
		vShaderFile.open(TAGResourceManager::asset_path + vertexPath);
		fShaderFile.open(TAGResourceManager::asset_path + fragmentPath);
		std::stringstream vShaderStream, fShaderStream;
		// read file's buffer contents into streams
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		// close file handlers
		vShaderFile.close();
		fShaderFile.close();
		// convert stream into string
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}

	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();

	// Compile shaders
	unsigned int vertex, fragment;
	int success;
	char infoLog[512];

	vertex = TAGResourceManager::createBuffer<OpenGLVertexShader>();
	glShaderSource(vertex, 1, &vShaderCode, NULL);
	glCompileShader(vertex);
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertex, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	};

	fragment = TAGResourceManager::createBuffer<OpenGLFragmentShader>();
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragment, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	};

	// shader Program
	const unsigned int ID = TAGResourceManager::createBuffer<OpenGLProgram>();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	glLinkProgram(ID);
	glGetProgramiv(ID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(ID, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	TAGResourceManager::deleteBuffer<OpenGLVertexShader>(vertex);
	TAGResourceManager::deleteBuffer<OpenGLFragmentShader>(fragment);
	return ID;
}
void TAGShaderManager::addShader(const Source& source) {
	shaders.try_emplace(source.name, loadShader(source.vertexPath, source.fragmentPath));
}
void TAGShaderManager::addShader(const std::vector<Source>& sources) {
	for (const Source& source : sources) {
		shaders.try_emplace(source.name, loadShader(source.vertexPath, source.fragmentPath));
	}
}
void TAGShaderManager::deleteShader(const std::string& name) {
	TAGResourceManager::deleteBuffer<OpenGLProgram>(shaders.at(name).ID);
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
void TAGShaderManager::Shader::setBool(const std::string& name, const bool& value, const unsigned int& count) const {
	glUniform1iv(glGetUniformLocation(ID, name.c_str()), count, (GLint*)&value);
}
void TAGShaderManager::Shader::setInt(const std::string& name, const int& value, const unsigned int& count) const {
	glUniform1iv(glGetUniformLocation(ID, name.c_str()), count, (GLint*)&value);
}
void TAGShaderManager::Shader::setFloat(const std::string& name, const float& value, const unsigned int& count) const {
	glUniform1fv(glGetUniformLocation(ID, name.c_str()), count, &value);
}
void TAGShaderManager::Shader::setVec4(const std::string& name, const glm::vec4& value, const unsigned int& count) const {
	glUniform4fv(glGetUniformLocation(ID, name.c_str()), count, (GLfloat*)&value);
}
void TAGShaderManager::Shader::setVec3(const std::string& name, const glm::vec3& value, const unsigned int& count) const {
	glUniform3fv(glGetUniformLocation(ID, name.c_str()), count, (GLfloat*)&value);
}
void TAGShaderManager::Shader::setMatrix4(const std::string& name, const glm::mat4& value, const unsigned int& count) const {
	glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), count, GL_FALSE, glm::value_ptr(value));
}
void TAGShaderManager::Shader::setMatrix3(const std::string& name, const glm::mat3& value, const unsigned int& count) const {
	glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), count, GL_FALSE, glm::value_ptr(value));
}