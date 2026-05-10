#pragma once

#include <ShaderManagerClass.hpp>

template<UniformType T> void TAGShaderManager::Shader::set(const std::string& name, const T& value, const unsigned int& count) const {
	if (name == "") {
		return;
	}

	if constexpr (std::same_as<T, bool>) {
		glUniform1iv(glGetUniformLocation(ID, name.c_str()), count, (GLint*)&value);
	} 
	else if constexpr (std::same_as<T, int>) {
		glUniform1iv(glGetUniformLocation(ID, name.c_str()), count, (GLint*)&value);
	} 
	else if constexpr (std::same_as<T, float>) {
		glUniform1fv(glGetUniformLocation(ID, name.c_str()), count, &value);
	}
	else if constexpr (std::same_as<T, glm::vec4>) {
		glUniform4fv(glGetUniformLocation(ID, name.c_str()), count, (GLfloat*)&value);
	}
	else if constexpr (std::same_as<T, glm::vec3>) {
		glUniform3fv(glGetUniformLocation(ID, name.c_str()), count, (GLfloat*)&value);
	}
	else if constexpr (std::same_as<T, glm::mat4>) {
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), count, GL_FALSE, glm::value_ptr(value));
	}
	else {
		glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), count, GL_FALSE, glm::value_ptr(value));
	}
}