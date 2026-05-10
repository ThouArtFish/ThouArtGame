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
#include "UtilClass.hpp"

/**
* Default shaders
*/
enum class TAGDefaultShader {
    SKYBOX_DRAW = 0,
    BASIC_DRAW,
    HUD_DRAW,
    UNINSTANCED_MODEL_DRAW,
    INSTANCED_MODEL_DRAW
};

/**
* Concept for types allowed to be set as shader uniforms
*/
template<typename T> concept UniformType = isAnyOf<T, bool, int, float, glm::vec4, glm::vec3, glm::mat4, glm::mat3>;

/**
* Handles shader programs
*/
class TAGShaderManager {
public:
    /**
    * Container for a shader program's source 
    */
    struct Source {
        std::string vertex;
        std::string fragment;
        std::string name;
        bool is_path = true;
    };
    /**
    * Holds the ID for a shader program and functions for setting shader uniforms
    */
    struct Shader {
        unsigned int ID;

        template<UniformType T> void set(const std::string& name, const T& value, const unsigned int& count) const;
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
    * The shader program is active until useShader activates a different shader program
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
    static constexpr unsigned int DEFAULT_SHADER_COUNT = 8;
    static constexpr std::string_view shader_version = "#version 460 core\n";
    static constexpr std::array<std::string_view, DEFAULT_SHADER_COUNT> default_source = {
        // Uninstanced vertex 0
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoords;\n"
        "out vec2 TexCoords;\n"
        "out vec3 Normal;\n"
        "out vec3 FragPos;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 perspective;\n"
        "uniform mat3 normal;\n"
        "void main() {\n"
        "   TexCoords = aTexCoords;\n"
        "   Normal = normal * aNormal;\n"
        "   vec4 game_pos = model * vec4(aPos, 1.0f);\n"
        "   FragPos = vec3(game_pos);\n"
        "   gl_Position = perspective * view * game_pos;\n}",
        // Instanced vertex 1
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoords;\n"
        "layout (location = 3) in vec4 ModelPosScale;\n"
        "layout (location = 4) in vec4 ModelAxisRot;\n"
        "out vec2 TexCoords;\n"
        "out vec3 Normal;\n"
        "out vec3 FragPos;\n"
        "uniform mat4 view;\n"
        "uniform mat4 perspective;\n"
        "vec3 axisRotation(vec3 v, vec3 a, float r) {\n"
        "   return v * cos(r) + cross(a, v) * sin(r) + a * dot(a, v) * (1.0f - cos(r));\n}"
        "void main() {\n"
        "   TexCoords = aTexCoords;\n"
        "   Normal = axisRotation(aNormal, ModelAxisRot.xyz, ModelAxisRot.w);\n"
        "   FragPos = axisRotation(aPos, ModelAxisRot.xyz, ModelAxisRot.w) * ModelPosScale.w + ModelPosScale.xyz;\n"
        "   gl_Position = perspective * view * vec4(FragPos, 1.0f);\n}",
        // Skybox vertex 2
        "layout (location = 0) in vec3 aPos;\n"
        "out vec3 TexCoords;\n"
        "uniform mat4 view;\n"
        "uniform mat4 perspective;\n"
        "void main() {\n"
        "   TexCoords = aPos;\n"
        "   vec4 pos = perspective * view * vec4(aPos, 1.0);\n"
        "   gl_Position = pos.xyww;\n}",
        // HUD Instanced vertex 3
        "layout (location = 0) in vec2 Base;\n"
        "layout (location = 1) in vec4 QuadData;\n"
        "layout (location = 2) in vec4 TexelData;\n"
        "layout (location = 3) in int TexInd;\n"
        "out vec2 TexCoord;\n"
        "flat out int tex_index;\n"
        "void main() {\n"
        "   tex_index = TexInd;\n"
        "   TexCoord = Base * TexelData.zw + TexelData.xy;\n"
        "   gl_Position = vec4(vec2(Base.x, Base.y - 1.0f) * QuadData.zw + QuadData.xy, 0.0f, 1.0f);\n}",
        // Skybox fragment 4
        "out vec4 FragColor;\n"
        "in vec3 TexCoords;\n"
        "uniform samplerCube skybox;\n"
        "void main() {\n"
            "FragColor = texture(skybox, TexCoords);\n}",
        // Basic fragment 5
        ""
    };
    std::unordered_map<std::string, Shader> shaders;

    static unsigned int loadShader(Source source);
    static void loadFromFile(Source& source);
};

#include "../../src/ResourceManagerClass.inl";