#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <array>
#include <unordered_map>
#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "BaseStateClass.hpp"
#include "ResourceManagerClass.hpp"

/**
* Handles shader programs
*/
class TAGShaderManager {
public:
    /**
    * Container for a shader program's source 
    */
    struct Source {
        const std::string vertexPath;
        const std::string fragmentPath;
        const std::string name;
    };
    /**
    * Holds the ID for a shader program and functions for setting shader uniforms
    */
    struct Shader {
        unsigned int ID;
        void setBool(const std::string& name, const bool& value) const;
        void setInt(const std::string& name, const int& value) const;
        void setFloat(const std::string& name, const float& value) const;
        void setVec4(const std::string& name, const glm::vec4& value) const;
        void setVec3(const std::string& name, const glm::vec3& value) const;
        void setMatrix4(const std::string& name, const glm::mat4& value) const;
        void setMatrix3(const std::string& name, const glm::mat3& value) const;
    };
    bool delete_on_death = true;

    /**
    * Pass one or multiple shader program sources (stored within a vector) to load them.
    * 
    * @param source(s) Source structs containing shader program component paths
    */
    TAGShaderManager(const Source& source);
    TAGShaderManager(const std::vector<Source>& sources);
    ~TAGShaderManager();

    /**
    * Add new shader program(s)
    * 
    * @param source(s) Source struct(s) containing shader program component paths
    */
    void addShader(const Source& source);
    void addShader(const std::vector<Source>& sources);
    /**
    * Delete shader program(s), identifying by name.
    * 
    * @param name(s) Name(s) of shader program(s)
    */
    void deleteShader(const std::string& name);
    void deleteShader(const std::vector<std::string>& names);
    /**
    * Activates a shader program and returns a reference to allow uniforms to be set.
    * The shader program is activated until useShader activates a different shader program
    * or stopShader deactivates the current program.
    * 
    * @param name The name of the shader program
    */
    const Shader& useShader(const std::string& name) const;
    /**
    * Deactivates the current shader.
    */
    void stopShader() const;
    /**
    * Gets all shader program names.
    */
    std::vector<std::string> getShaderNames() const;
    auto begin() const;
    auto end() const;
private:
    std::unordered_map<std::string, Shader> shaders;

    static unsigned int loadShader(const std::string& vertex_path, const std::string& fragment_path);
};
