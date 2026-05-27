#pragma once

#include <ShaderManagerClass.hpp>

template<typename T, typename V> requires isVariantMember<T, V> void TAGShaderManager::Shader::set(const std::string& name, const T& value, const unsigned int& count) const {
    if (uniform_data.find(name) == uniform_data.end() || getEnumType<T>() != uniform_data.at(name).data_type) return;

    const GLint& loc = uniform_data.at(name).location;

    if constexpr (std::same_as<T, bool> || std::same_as<T, int>) {
        glUniform1iv(loc, count, (GLint*)&value);
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

template<typename T, typename V> requires isVariantMember<T, V> GLenum TAGShaderManager::getEnumType() {
    if constexpr (std::same_as<T, bool> || std::same_as<T, int>) {
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