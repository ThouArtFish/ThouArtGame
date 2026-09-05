#pragma once

#include <ShaderManagerClass.hpp>

template<ShaderUniformType::Concept T> void TAGShaderManager::Shader::set(const std::string& name, const T& value, const GLuint& count) const {
    if (uniform_data.find(name) == uniform_data.end() || getEnumType<T>() != uniform_data.at(name).data_type) {
        return;
    }

    const GLint& loc = uniform_data.at(name).location;

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

    for (GLuint i = 0; i < shader_names.size(); i++) {
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