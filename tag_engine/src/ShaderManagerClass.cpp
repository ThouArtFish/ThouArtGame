#include <ShaderManagerClass.hpp>

TAGShaderManager::TAGShaderManager() {}

TAGShaderManager::~TAGShaderManager() {
    for (const auto& pair : shaders) TAGResourceManager::deleteBuffer<OpenGLObjectType::ShaderProgram>(pair.second.ID);
}

void TAGShaderManager::deleteShader(const std::string& name) {
    if (shaders.erase(name) > 0) TAGResourceManager::deleteBuffer<OpenGLObjectType::ShaderProgram>(shaders.at(name).ID);
}

void TAGShaderManager::deleteShader(const std::vector<std::string>& names) {
    for (const std::string& name : names) deleteShader(name);
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

GLuint TAGShaderManager::compileProgram(const std::vector<GLuint>& shader_ids) {
    int success;
    const GLuint ID = TAGResourceManager::createBuffer<OpenGLObjectType::ShaderProgram>();
    for (GLuint shader_id : shader_ids) if (shader_id > 0) glAttachShader(ID, shader_id);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        std::array<GLchar, 512> info_log;
        glGetProgramInfoLog(ID, 512, NULL, &info_log[0]);
        std::cout << "TAGShaderManager : Shader program could not be linked:\n" + std::string(&info_log[0], 512) << std::endl;
        TAGResourceManager::deleteBuffer<OpenGLObjectType::ShaderProgram>(ID);
        return 0;
    }
    return ID;
}

void TAGShaderManager::getSourceCodeFromFile(std::string& source_code, const std::string& path) {
    std::ifstream file;
    std::stringstream stream;

    // Read source code from shader files and except errors
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        file.open(TAGResourceManager::asset_path + path);
        stream << file.rdbuf();
        file.close();
        source_code = stream.str();
    }
    catch (std::ifstream::failure e)
    {
        std::cout << "TAGShaderManager : Shader source code files could not be read" << std::endl;
    }
}

void TAGShaderManager::getSourceCodeFromDefault(DefaultProgramType type, std::string& vertex_code, std::string& fragment_code) {
    vertex_code = fragment_code = shader_version.substr();
    switch (type) {
    case DefaultProgramType::SKYBOX_DRAW:
        vertex_code += default_source[2].substr();
        fragment_code += default_source[4].substr();
        break;
    case DefaultProgramType::UNINSTANCED_BASIC_DRAW:
        vertex_code += default_source[0].substr();
        fragment_code += default_source[5].substr();
        break;
    case DefaultProgramType::INSTANCED_BASIC_DRAW:
        vertex_code += default_source[1].substr();
        fragment_code += default_source[5].substr();
        break;
    case DefaultProgramType::HUD_DRAW:
        vertex_code += default_source[3].substr();
        fragment_code += default_source[6].substr();
        break;
    case DefaultProgramType::UNINSTANCED_MODEL_DRAW:
        vertex_code += default_source[0].substr();
        fragment_code += default_source[7].substr();
        break;
    case DefaultProgramType::SKYBOX_SETUP:
        vertex_code += default_source[2].substr();
        fragment_code += default_source[8].substr();
    default: // INSTANCED_MODEL_DRAW
        vertex_code += default_source[1].substr();
        fragment_code += default_source[7].substr();
    }
}
