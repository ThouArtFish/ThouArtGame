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
* Shader uniform sampler types
*/
namespace ShaderUniformType {
    struct TagType {};

    struct SINGLE_2D : SameType<int>, TagType {
        SINGLE_2D(const int& num) : SameType(num) {}
        static constexpr GLenum ENUM = GL_SAMPLER_2D;
    };

    struct CUBEMAP : SameType<int>, TagType {
        CUBEMAP(const int& num) : SameType(num) {}
        static constexpr GLenum ENUM = GL_SAMPLER_CUBE;
    };

    using Primitive = std::variant<
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

    template<typename T> concept Concept = isVariantMember<T, Primitive> || (!std::same_as<T, TagType> && std::derived_from<T, TagType>);
};

/**
* Handles shader programs
*/
class TAGShaderManager : public TAGBaseState::OpenGLContextChecker {
public:
    /**
    * Shader types
    */
    enum class ShaderType {
        SKYBOX_DRAW = 0,
        UNINSTANCED_BASIC_DRAW,
        INSTANCED_BASIC_DRAW,
        HUD_DRAW,
        UNINSTANCED_MODEL_DRAW,
        INSTANCED_MODEL_DRAW,
        CUSTOM_DRAW
    };

    /**
    * Shader variable data
    */
    struct ShaderAttributeInfo {
        std::string name;
        GLenum data_type;
    };
    struct ShaderUniformInfo {
        GLint location;
        GLenum data_type;
    };

    /**
    * Container for a shader program's source 
    */
    struct Source {
        std::string vertex;
        std::string fragment;
        std::string name;
        bool is_path = true;
        ShaderType shader_type = ShaderType::CUSTOM_DRAW;
    };

    /**
    * Holds the ID for a shader program and functions for setting shader uniforms
    */
    struct Shader {
        GLuint ID;
        std::unordered_map<GLint, ShaderAttributeInfo> attribute_data;
        std::unordered_map<std::string, ShaderUniformInfo> uniform_data;
        std::unordered_map<GLuint, std::vector<GLint>> buffer_locations;

        /**
        * Set named uniform in shader
        * 
        * @param name Name of uniform
        * @param value Value to set to uniform
        * @param count Number of individual values to set, default is 1
        */
        template<ShaderUniformType::Concept T> void set(const std::string& name, const T& value, const GLuint& count = 1) const;
    };

    /**
    * Default shader uniform names
    */
    struct ShaderOptions {
        std::string perspective_mat = "perspective";
        std::string camera_mat = "view";
        std::string camera_pos = "camera_pos";
        std::string camera_dir = "camera_dir";
        std::string shader_object = "object";
        std::string colour_vec = "colour";
        std::string opacity_value = "opacity";
        std::string cubemap = "cubemap";
        std::string specular_exp = "spec_exp";
        std::string specular_factor = "spec_fac";
        std::string diffuse_tex_num = "diff_tex_num";
        std::string specular_tex_num = "spec_tex_num";
        std::string diffuse_tex_array = "diff_texs";
        std::string specular_tex_array = "spec_texs";
        bool cull_backface = true;
    };

    static inline ShaderOptions default_options = {};

    /**
    * Pass one or multiple shader program sources (stored within a vector) to load them.
    * 
    * @param source(s) Source structs containing shader program component paths
    */
    TAGShaderManager();
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
    * For each shader named in shader_names, sets the corresponding value in values to the corresponding uniform in 
    * uniform_names. If the length of uniform_names or values is less than shader_names, then the last value in each
    * vector is used for the remaining shaders.
    * 
    * @param shader_names Names of each shader
    * @param uniform_names Names of uniforms in each shader
    * @param values Values to set to uniforms
    * @param count Number of individual values to set, default is 1
    */
    template<ShaderUniformType::Concept T> void setAll(const std::vector<std::string>& shader_names, const std::vector<std::string>& uniform_names, const std::vector<T>& values, const GLuint& count = 1) const;
    /**
    * Activates a shader program and returns a reference to allow uniforms to be set.
    * The shader program is active until useShader activates a different shader program
    * or stopShader deactivates the current program.
    * 
    * @param name The name of the shader program
    */
    const Shader& useShader(const std::string& name) const;
    /**
    * Gets all shader program names.
    */
    std::vector<std::string> getShaderNames() const;
    /**
    * Deactivates the current shader.
    */
    static void stopShader();
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
        "uniform mat4 view;\n"
        "uniform mat4 perspective;\n"
        "uniform vec4 object[2];\n"
        "vec3 axisRotation(vec3 v, vec3 a, float r) {\n"
        "   return v * cos(r) + cross(a, v) * sin(r) + a * dot(a, v) * (1.0f - cos(r));\n}\n"
        "void main() {\n"
        "   TexCoords = aTexCoords;\n"
        "   Normal = axisRotation(aNormal, object[1].xyz, object[1].w);\n"
        "   FragPos = axisRotation(aPos, object[1].xyz, object[1].w) * object[0].w + object[0].xyz;\n"
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
        "uniform samplerCube cubemap;\n"
        "void main() {\n"
        "   FragColor = texture(cubemap, TexCoords);\n}",
        // Basic fragment 5
        "out vec4 FragColour;\n"
        "in vec2 TexCoords;\n"
        "in vec3 Normal;\n"
        "in vec3 FragPos;\n"
        "uniform int diff_tex_num;\n"
        "uniform vec3 colour;\n"
        "uniform sampler2D diff_texs[16];\n"
        "void main() {\n"
        "   if (diff_tex_num == 0) {\n"
        "      FragColour = vec4(colour, 1.0f);\n"
        "   } else {\n"
        "      FragColour = texture(diff_texs[0], TexCoords);\n"
        "      float factor = 1.0f / float(diff_tex_num);\n"
        "      for (int i = 1; i < diff_tex_num - 1; i++) {\n"
        "          FragColour = mix(FragColour, texture(diff_texs[i], TexCoords), factor);\n"
        "      }\n"
        "   }\n}",
        // HUD fragment 6
        "out vec4 FragColour;\n"
        "in vec2 TexCoord;\n"
        "flat in int tex_index;\n"
        "uniform sampler2D diff_texs[16];\n"
        "void main() {\n"
        "   FragColour = texture(diff_texs[tex_index], TexCoord);\n"
        "   if (FragColour.w < 0.01f)\n"
        "      discard;\n}",
        // Object fragment 7
        "struct PointLight {\n"
        "    vec4 a, b;\n"
        "};\n"
        "struct RayLight {\n"
        "    vec4 a;\n"
        "    vec2 b;\n"
        "};\n"
        "struct FlashLight {\n"
        "    vec4 a, b, c;\n"
        "};\n"
        "struct Scene {\n"
        "    float ambience;\n"
        "};\n"
        "layout(std430, binding = 0) readonly buffer ssbo_0 {\n"
        "    int point_lights_size;\n"
        "    PointLight point_lights[];\n"
        "};\n"
        "layout(std430, binding = 1) readonly buffer ssbo_1 {\n"
        "    int ray_lights_size;\n"
        "    RayLight ray_lights[];\n"
        "};\n"
        "layout(std430, binding = 2) readonly buffer ssbo_2 {\n"
        "    int flash_lights_size;\n"
        "    FlashLight flash_lights[];\n"
        "};\n"
        "layout(std430, binding = 3) readonly buffer ssbo_3 {\n"
        "    Scene scene;\n"
        "};\n"
        "out vec4 FragColour;\n"
        "in vec2 TexCoords;\n"
        "in vec3 Normal;\n"
        "in vec3 FragPos;\n"
        "uniform vec3 camera_pos;\n"
        "uniform vec3 colour;\n"
        "uniform float spec_fac;\n"
        "uniform float spec_exp;\n"
        "uniform float opacity;\n"
        "uniform int spec_tex_num;\n"
        "uniform int diff_tex_num;\n"
        "uniform sampler2D diff_texs[16];\n"
        "uniform sampler2D spec_texs[16];\n"
        "void main() {\n"
        "    int i;\n"
        "    vec3 final_shade = vec3(scene.ambience);\n"
        "    vec4 obj_base = (diff_tex_num == 0 ? vec4(colour, 1.0f) : texture(diff_texs[0], TexCoords));\n"
        "    if (obj_base.w < 0.001f) discard;\n"
        "    vec3 spec_frag = (spec_tex_num == 0 ? vec3(spec_fac) : texture(spec_texs[0], TexCoords).xyz);\n"
        "    for (i = 0; i < point_lights_size; i++) {\n"
        "        vec3 light_dir = point_lights[i].a.xyz - FragPos;\n"
        "        float dist = length(light_dir);\n"
        "        float atten = 1.0f / (1.0f + point_lights[i].a.w * dist + point_lights[i].b.w * dist * dist);\n"
        "        light_dir /= dist;\n"
        "        final_shade += (\n"
        "            point_lights[i].b.xyz * max(0, dot(Normal, light_dir)) +\n"
        "            point_lights[i].b.xyz * spec_frag * pow(max(dot(Normal, normalize(normalize(camera_pos - FragPos) + light_dir)), 0.0), spec_exp)\n"
        "        ) * atten;\n"
        "    }\n"
        "    for (i = 0; i < ray_lights_size; i++) {\n"
        "        vec3 colour = vec3(ray_lights[i].a.z, ray_lights[i].b);\n"
        "        final_shade += colour * max(0, dot(Normal, ray_lights[i].a.xyz)) + colour * spec_frag * pow(max(dot(Normal, normalize(normalize(camera_pos - FragPos) + ray_lights[i].a.xyz)), 0.0), spec_exp);\n"
        "    }\n"
        "    for (i = 0; i < flash_lights_size; i++) {\n"
        "        vec3 light_dir = flash_lights[i].a.xyz - FragPos;\n"
        "        vec3 view_dir = normalize(camera_pos - FragPos);\n"
        "        float dist = length(light_dir);\n"
        "        float atten = 1.0f / (1.0f + flash_lights[i].a.w * dist + flash_lights[i].b.w * dist * dist);\n"
        "        light_dir /= dist;\n"
        "        vec3 colour = flash_lights[i].c.xyz * min(1, max(0, (dot(-light_dir, flash_lights[i].b.xyz) - flash_lights[i].c.w) / (flash_lights[i].c.w - cos(acos(flash_lights[i].c.w) + 0.1396))));\n"
        "        final_shade += (\n"
        "            colour * max(0, dot(Normal, flash_lights[i].b.xyz)) +\n"
        "            colour * spec_frag * pow(max(dot(Normal, normalize(normalize(camera_pos - FragPos) + flash_lights[i].b.xyz)), 0.0), spec_exp)\n"
        "        ) * atten;\n"
        "    }\n"
        "    FragColour = vec4(obj_base.xyz * final_shade, obj_base.w * opacity);\n}"
    };
    std::unordered_map<std::string, Shader> shaders;

    static Shader loadShader(const Source& source);
    static void getSourceCodeFromFile(const Source& source, std::string& vertex_code, std::string& fragment_code);
    static void getSourceCodeFromDefault(const Source&, std::string& vertex_code, std::string& fragment_code);
    template<ShaderUniformType::Concept T> static GLenum getEnumType();
};

#include "../../src/ShaderManagerClass.inl"
