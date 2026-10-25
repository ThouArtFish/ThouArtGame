#pragma once

#include <ShaderManagerClass.hpp>

template<ProgramType::Concept T> void TAGShaderManager::addShader(const T& source) {
    // Fetch from files or default source code
    std::string vertex_code, fragment_code, geometry_code, compute_code;
    if constexpr (std::same_as<T, ProgramType::VGF>) {
        getSourceCodeFromFile(vertex_code, source.vertex_path);
        getSourceCodeFromFile(fragment_code, source.fragment_path);
        if (!source.geometry_path.empty()) getSourceCodeFromFile(geometry_code, source.geometry_path);
    }
    if constexpr (std::same_as<T, ProgramType::Compute>) {
        getSourceCodeFromFile(compute_code, source.compute_path);
    }
    else { // ProgramType::Default
        getSourceCodeFromDefault(source.type, vertex_code, fragment_code);
        if (!source.geometry_path.empty()) getSourceCodeFromFile(geometry_code, source.geometry_path);
    }

    // Compile program stages
    std::vector<GLuint> shader_ids(4);
    if (!vertex_code.empty()) shader_ids[0] = compileShader<OpenGLObjectType::VertexShader>(vertex_code);
    if (!fragment_code.empty()) shader_ids[1] = compileShader<OpenGLObjectType::FragmentShader>(fragment_code);
    if (!geometry_code.empty()) shader_ids[2] = compileShader<OpenGLObjectType::GeometryShader>(geometry_code);
    if (!compute_code.empty()) shader_ids[3] = compileShader<OpenGLObjectType::ComputeShader>(geometry_code);

    // Compile final shader program
    shaders.insert_or_assign(source.name, Shader{ .ID = compileProgram(shader_ids) });
    Shader& shader = shaders[source.name];
    const GLuint ID = shader.ID;

    // Delete shaders
    if (!vertex_code.empty()) TAGResourceManager::deleteBuffer<OpenGLObjectType::VertexShader>(shader_ids[0]);
    if (!fragment_code.empty()) TAGResourceManager::deleteBuffer<OpenGLObjectType::FragmentShader>(shader_ids[1]);
    if (!geometry_code.empty()) TAGResourceManager::deleteBuffer<OpenGLObjectType::GeometryShader>(shader_ids[2]);
    if (!compute_code.empty()) TAGResourceManager::deleteBuffer<OpenGLObjectType::ComputeShader>(shader_ids[3]);

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

        glGetActiveUniform(ID, i, (GLsizei)data.max_size(), &length, &size, &type, chars);

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

        glGetActiveAttrib(ID, i, (GLsizei)data.max_size(), &length, &size, &type, (GLchar*)data.data());

        GLint location = glGetAttribLocation(ID, (GLchar*)data.data());

        shader.attribute_data[location] = { std::string((GLchar*)data.data(), length), type };
    }

    // Get data for each program interface
    constexpr std::array<GLenum, 2> props = { GL_BUFFER_BINDING, GL_NUM_ACTIVE_VARIABLES };
    for (const TAGResourceManager::ShaderBufferInterfaceType& type : TAGResourceManager::buffer_interface_types) {
        glGetProgramInterfaceiv(ID, (GLenum)type, GL_ACTIVE_RESOURCES, &count);
        for (GLint i = 0; i < count; i++) {
            GLsizei length;
            glGetProgramResourceiv(ID, (GLenum)type, i, (GLsizei)props.max_size(), props.data(), sizeof(data), &length, data.data());

            shader.buffer_locations[(GLuint)TAGResourceManager::interfaceToBufferType(type)].push_back(data[0]);
        }
    }
}

template<ProgramType::Concept T> void TAGShaderManager::addShader(const std::vector<T>& sources) {
    for (const T& source : sources) addShader(source);
}

template<OpenGLObjectType::Concept T> GLuint TAGShaderManager::compileShader(const std::string& source_code) {
    int success;
    std::vector<GLchar*> source_ptr{ (GLchar*)source_code.c_str() };
    const GLuint shader_id = TAGResourceManager::createBuffer<T>();
    glShaderSource(shader_id, 1, source_ptr.data(), NULL);
    glCompileShader(shader_id);
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
    if (!success) {
        std::array<GLchar, 512> info_log;
        glGetShaderInfoLog(shader_id, 512, NULL, &info_log[0]);
        std::cout << "TAGShaderManager : Compilation of shader failed:\n" << std::string(&info_log[0], 512) << std::endl;
        TAGResourceManager::deleteBuffer<T>(shader_id);
        return 0;
    };
    return shader_id;
}

template<ShaderUniformType::Concept T> void TAGShaderManager::Shader::set(const std::string& name, const T& value, const GLuint& count) const {
    if (uniform_data.find(name) == uniform_data.end() || getEnumType<T>() != uniform_data.at(name).data_type) return;

    const GLint loc = uniform_data.at(name).location;

    if constexpr (std::same_as<T, bool>) {
        const GLint conv = (value ? 1 : 0);
        glUniform1iv(loc, count, &conv);
    }
    else if constexpr (std::same_as<T, int>) {
        glUniform1iv(loc, count, (GLint*)&value);
    }
    else if constexpr (std::derived_from<T, ShaderUniformType::TagType>) {
        glUniform1iv(loc, count, (GLint*)&value.value);
    }
    else if constexpr (std::same_as<T, unsigned int>) {
        glUniform1uiv(loc, count, (GLuint*)&value);
    }
    else if constexpr (std::same_as<T, float>) {
        glUniform1fv(loc, count, (GLfloat*)&value);
    }
    else if constexpr (std::same_as<T, glm::vec2>) {
        glUniform2fv(loc, count, (GLfloat*)&value);
    }
    else if constexpr (std::same_as<T, glm::vec3>) {
        glUniform3fv(loc, count, (GLfloat*)&value);
    }
    else if constexpr (std::same_as<T, glm::vec4>) {
        glUniform4fv(loc, count, (GLfloat*)&value);
    }
    else if constexpr (std::same_as<T, glm::ivec2> || std::same_as<T, glm::bvec2>) {
        glUniform2iv(loc, count, (GLint*)&value);
    }
    else if constexpr (std::same_as<T, glm::ivec3> || std::same_as<T, glm::bvec3>) {
        glUniform3iv(loc, count, (GLint*)&value);
    }
    else if constexpr (std::same_as<T, glm::ivec4> || std::same_as<T, glm::bvec4>) {
        glUniform4iv(loc, count, (GLint*)&value);
    }
    else if constexpr (std::same_as<T, glm::uvec2>) {
        glUniform2uiv(loc, count, (GLuint*)&value);
    }
    else if constexpr (std::same_as<T, glm::uvec3>) {
        glUniform3uiv(loc, count, (GLuint*)&value);
    }
    else if constexpr (std::same_as<T, glm::uvec4>) {
        glUniform4uiv(loc, count, (GLuint*)&value);
    }
    else if constexpr (std::same_as<T, glm::mat2>) {
        glUniformMatrix2fv(loc, count, GL_FALSE, glm::value_ptr(value));
    }
    else if constexpr (std::same_as<T, glm::mat3>) {
        glUniformMatrix3fv(loc, count, GL_FALSE, glm::value_ptr(value));
    }
    else if constexpr (std::same_as<T, glm::mat4>) {
        glUniformMatrix4fv(loc, count, GL_FALSE, glm::value_ptr(value));
    }
    else if constexpr (std::same_as<T, glm::mat2x3>) {
        glUniformMatrix2x3fv(loc, count, GL_FALSE, glm::value_ptr(value));
    }
    else if constexpr (std::same_as<T, glm::mat2x4>) {
        glUniformMatrix2x4fv(loc, count, GL_FALSE, glm::value_ptr(value));
    }
    else if constexpr (std::same_as<T, glm::mat3x2>) {
        glUniformMatrix3x2fv(loc, count, GL_FALSE, glm::value_ptr(value));
    }
    else if constexpr (std::same_as<T, glm::mat3x4>) {
        glUniformMatrix3x4fv(loc, count, GL_FALSE, glm::value_ptr(value));
    }
    else if constexpr (std::same_as<T, glm::mat4x2>) {
        glUniformMatrix4x2fv(loc, count, GL_FALSE, glm::value_ptr(value));
    }
    else { // std::same_as<T, glm::mat4x3>
        glUniformMatrix4x3fv(loc, count, GL_FALSE, glm::value_ptr(value));
    }
}

template<ShaderUniformType::Concept T> void TAGShaderManager::setAll(const std::vector<std::string>& shader_names, const std::vector<std::string>& uniform_names, const std::vector<T>& values, const GLuint& count) const {
    if (uniform_names.empty() || values.empty()) return;

    for (size_t i = 0; i < shader_names.size(); i++) {
        useShader(shader_names[i]).set<T>(
            (i < uniform_names.size() ? uniform_names[i] : uniform_names.back()), 
            (i < values.size() ? values[i] : values.back()),
            count
        );
    }
}

template<ShaderUniformType::Concept T> GLenum TAGShaderManager::getEnumType() {
    if constexpr (std::derived_from<T, ShaderUniformType::TagType>) {
        return T::ENUM;
    }
    else if constexpr (std::same_as<T, bool> || std::same_as<T, int>) {
        return GL_INT;
    }
    else if constexpr (std::same_as<T, unsigned int>) {
        return GL_UNSIGNED_INT;
    }
    else if constexpr (std::same_as<T, float>) {
        return GL_FLOAT;
    }
    else if constexpr (std::same_as<T, glm::vec2>) {
        return GL_FLOAT_VEC2;
    }
    else if constexpr (std::same_as<T, glm::vec3>) {
        return GL_FLOAT_VEC3;
    }
    else if constexpr (std::same_as<T, glm::vec4>) {
        return GL_FLOAT_VEC4;
    }
    else if constexpr (std::same_as<T, glm::ivec2> || std::same_as<T, glm::bvec2>) {
        return GL_INT_VEC2;
    }
    else if constexpr (std::same_as<T, glm::ivec3> || std::same_as<T, glm::bvec3>) {
        return GL_INT_VEC3;
    }
    else if constexpr (std::same_as<T, glm::ivec4> || std::same_as<T, glm::bvec4>) {
        return GL_INT_VEC4;
    }
    else if constexpr (std::same_as<T, glm::uvec2>) {
        return GL_UNSIGNED_INT_VEC2;
    }
    else if constexpr (std::same_as<T, glm::uvec3>) {
        return GL_UNSIGNED_INT_VEC3;
    }
    else if constexpr (std::same_as<T, glm::uvec4>) {
        return GL_UNSIGNED_INT_VEC4;
    }
    else if constexpr (std::same_as<T, glm::mat2>) {
        return GL_FLOAT_MAT2;
    }
    else if constexpr (std::same_as<T, glm::mat3>) {
        return GL_FLOAT_MAT3;
    }
    else if constexpr (std::same_as<T, glm::mat4>) {
        return GL_FLOAT_MAT4;
    }
    else if constexpr (std::same_as<T, glm::mat2x3>) {
        return GL_FLOAT_MAT2x3;
    }
    else if constexpr (std::same_as<T, glm::mat2x4>) {
        return GL_FLOAT_MAT2x4;
    }
    else if constexpr (std::same_as<T, glm::mat3x2>) {
        return GL_FLOAT_MAT3x2;
    }
    else if constexpr (std::same_as<T, glm::mat3x4>) {
        return GL_FLOAT_MAT3x4;
    }
    else if constexpr (std::same_as<T, glm::mat4x2>) {
        return GL_FLOAT_MAT4x2;
    }
    else { // glm::mat4x3
        return GL_FLOAT_MAT4x3;
    }
}