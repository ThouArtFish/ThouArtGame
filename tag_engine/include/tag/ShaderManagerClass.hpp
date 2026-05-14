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
* Shader types
*/
enum class TAGShaderType {
    SKYBOX_DRAW = 0,
    BASIC_DRAW,
    HUD_DRAW,
    UNINSTANCED_MODEL_DRAW,
    INSTANCED_MODEL_DRAW,
    CUSTOM_DRAW
};

/**
* Variant for all shader primitives
*/
using ShaderUniform = std::variant<
    bool,
    int,
    unsigned int,
    float,
    glm::vec2,
    glm::vec3,
    glm::vec4,
    glm::ivec2,
    glm::ivec3,
    glm::ivec4,
    glm::uvec2,
    glm::uvec3,
    glm::uvec4,
    glm::bvec2,
    glm::bvec3,
    glm::bvec4,
    glm::mat2,
    glm::mat3,
    glm::mat4,
    glm::mat2x3,
    glm::mat2x4,
    glm::mat3x2,
    glm::mat3x4,
    glm::mat4x2,
    glm::mat4x3
>;

/**
* Concept for types allowed to be set as shader uniforms
*/
template<typename T> concept UniformType = isVariantMember<T, ShaderUniform>::value;

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
        TAGShaderType shader_type = TAGShaderType::CUSTOM_DRAW;
    };

    /**
    * Holds the ID for a shader program and functions for setting shader uniforms
    */
    struct Shader {
        unsigned int ID;

        template<UniformType T> void set(const std::string& name, const T& value, const unsigned int& count = 1) const;
    };

    /**
    * Default shader uniform names
    */
    struct ShaderOptions {
        std::string shader_object_name = "object";
        std::string colour_vec_name = "colour";
        std::string specular_colour_vec_name = "spec_colour";
        std::string opacity_value_name = "opacity";
        std::string cubemap_name = "cubemap";
        std::string specular_exp_name = "spec_exp";
        std::string specular_factor_name = "spec_fac";
        std::string diffuse_tex_num_name = "diff_tex_num";
        std::string specular_tex_num_name = "spec_tex_num";
        std::string diffuse_tex_array_name = "diff_texs";
        std::string specular_tex_array_name = "spec_texs";
        bool cull_backface = true;
    };

    static inline const ShaderOptions default_options = {};

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
        "struct ShaderObject {\n"
        "   vec4 position_AND_scale;\n"
        "   vec4 axis_AND_rotation;\n}\n"
        "uniform mat4 view;\n"
        "uniform mat4 perspective;\n"
        "uniform ShaderObject object;\n"
        "vec3 axisRotation(vec3 v, vec3 a, float r) {\n"
        "   return v * cos(r) + cross(a, v) * sin(r) + a * dot(a, v) * (1.0f - cos(r));\n}\n"
        "void main() {\n"
        "   TexCoords = aTexCoords;\n"
        "   Normal = axisRotation(aNormal, object.axis_AND_rotation.xyz, object.axis_AND_rotation.w);\n"
        "   FragPos = axisRotation(aPos, object.axis_AND_rotation.xyz, object.axis_AND_rotation.w) * object.position_AND_scale.w + object.position_AND_scale.xyz;\n"
        "   gl_Position = perspective * view * vec4(FragPos, 1.0f);\n}",
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
        "   return v * cos(r) + cross(a, v) * sin(r) + a * dot(a, v) * (1.0f - cos(r));\n}\n"
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

#include "../../src/ShaderManagerClass.inl";