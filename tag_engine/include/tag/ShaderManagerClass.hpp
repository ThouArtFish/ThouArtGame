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
        SINGLE_2D(int num) : SameType(num) {}
        static constexpr GLenum ENUM = GL_SAMPLER_2D;
    };

    struct CUBEMAP : SameType<int>, TagType {
        CUBEMAP(int num) : SameType(num) {}
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
* Possible program types
*/
namespace ProgramType {
    struct TagType {
        std::string name;
    };
    struct VGF : public TagType {
        std::string vertex_path, fragment_path, geometry_path;
    };
    struct Compute : public TagType {
        std::string compute_path;
    };
    struct Default;

    template<typename T> concept Concept = std::derived_from<T, TagType> && !std::same_as<T, TagType>;
};

/**
* Handles shader programs
*/
class TAGShaderManager : public TAGBaseState::OpenGLContextChecker {
public:
    /**
    * Default shader programs
    */
    enum class DefaultProgramType {
        SKYBOX_DRAW = 0,
        UNINSTANCED_BASIC_DRAW,
        INSTANCED_BASIC_DRAW,
        HUD_DRAW,
        UNINSTANCED_MODEL_DRAW,
        INSTANCED_MODEL_DRAW,
        SKYBOX_SETUP
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
        std::string ambience = "ambience";
        std::string camera_pos = "camera_pos";
        std::string camera_dir = "camera_dir";
        std::string shader_object = "object";
        std::string cubemap = "cubemap";
        std::string diffuse_tex_array = "diff_texs";
        std::string specular_tex_array = "spec_texs";
        bool cull_backface = true;
    };

    static inline ShaderOptions default_options = {};

    // Default shader version
    static constexpr std::string_view shader_version = "#version 460 core\n";

    // Default shader source code
    static constexpr std::array<std::string_view, 9> default_source = {
        // Uninstanced vertex 0
        "layout (location = 0) in vec3 MeshPosition;\n"
        "layout (location = 1) in vec3 MeshNormal;\n"
        "layout (location = 2) in vec2 MeshTexCoords;\n"
        "layout (location = 5) in vec3 MaterialRow1;\n"
        "layout (location = 6) in vec3 MaterialRow2;\n"
        "layout (location = 7) in uvec2 MaterialRow3;\n"
        "struct Material {\n"
        "   float spec_fac, spec_exp, opacity;\n"
        "   vec3 colour;\n"
        "   uint num_diffuse, num_specular;\n"
        "};\n"
        "out vec2 InterpMeshTexCoords;\n"
        "out vec3 InterpMeshNormal;\n"
        "out vec3 FragPosition;\n"
		"flat out Material Mat;\n"
        "uniform mat4 view;\n"
        "uniform mat4 perspective;\n"
        "uniform vec4 object[2];\n"
        "vec3 axisRotation(vec3 v, vec3 a, float r) {\n"
        "   return v * cos(r) + cross(a, v) * sin(r) + a * dot(a, v) * (1.0 - cos(r));\n}\n"
        "void main() {\n"
        "   Mat = Material(MaterialRow1.x, MaterialRow1.y, MaterialRow1.z, MaterialRow2, uint(MaterialRow3.x), uint(MaterialRow3.y));\n"
        "   InterpMeshTexCoords = MeshTexCoords;\n"
        "   InterpMeshNormal = axisRotation(MeshNormal, object[1].xyz, object[1].w);\n"
        "   FragPosition = axisRotation(MeshPosition, object[1].xyz, object[1].w) * object[0].w + object[0].xyz;\n"
        "   gl_Position = perspective * view * vec4(FragPosition, 1.0);\n}",
        // Instanced vertex 1
        "layout (location = 0) in vec3 MeshPosition;\n"
        "layout (location = 1) in vec3 MeshNormal;\n"
        "layout (location = 2) in vec2 MeshTexCoords;\n"
        "layout (location = 3) in vec4 ModelPosScale;\n"
        "layout (location = 4) in vec4 ModelAxisRot;\n"
		"layout (location = 5) in vec3 MaterialRow1;\n"
		"layout (location = 6) in vec3 MaterialRow2;\n"
		"layout (location = 7) in uvec2 MaterialRow3;\n"
		"struct Material {\n"
        "   float spec_fac, spec_exp, opacity;\n"
        "   vec3 colour;\n"
        "   uint num_diffuse, num_specular;\n"
        "};\n"
        "out vec2 InterpMeshTexCoords;\n"
        "out vec3 InterpMeshNormal;\n"
        "out vec3 FragPosition;\n"
		"flat out Material Mat;\n"
        "uniform mat4 view;\n"
        "uniform mat4 perspective;\n"
        "vec3 axisRotation(vec3 v, vec3 a, float r) {\n"
        "   return v * cos(r) + cross(a, v) * sin(r) + a * dot(a, v) * (1.0 - cos(r));\n}\n"
        "void main() {\n"
        "   Mat = Material(MaterialRow1.x, MaterialRow1.y, MaterialRow1.z, MaterialRow2, uint(MaterialRow3.x), uint(MaterialRow3.y));\n"
        "   InterpMeshTexCoords = MeshTexCoords;\n"
        "   InterpMeshNormal = axisRotation(MeshNormal, ModelAxisRot.xyz, ModelAxisRot.w);\n"
        "   FragPosition = axisRotation(MeshPosition, ModelAxisRot.xyz, ModelAxisRot.w) * ModelPosScale.w + ModelPosScale.xyz;\n"
        "   gl_Position = perspective * view * vec4(FragPosition, 1.0);\n}",
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
        "layout (location = 3) in uint TexInd;\n"
        "out vec2 TexCoord;\n"
        "flat out uint tex_index;\n"
        "void main() {\n"
        "   tex_index = TexInd;\n"
        "   TexCoord = Base * TexelData.zw + TexelData.xy;\n"
        "   gl_Position = vec4(vec2(Base.x, Base.y - 1.0) * QuadData.zw + QuadData.xy, 0.0, 1.0);\n}",
        // Skybox fragment 4
        "out vec4 FragColor;\n"
        "in vec3 TexCoords;\n"
        "uniform samplerCube cubemap;\n"
        "void main() {\n"
        "   FragColor = texture(cubemap, TexCoords);\n}",
        // Basic fragment 5
        "struct Material {\n"
        "   float spec_fac, spec_exp, opacity;\n"
        "   vec3 colour;\n"
        "   uint num_diffuse, num_specular;\n"
        "};\n"
        "out vec4 FragColour;\n"
        "in vec2 InterpMeshTexCoords;\n"
        "in vec3 InterpMeshNormal;\n"
        "in vec3 FragPosition;\n"
		"flat in Material Mat;\n"
        "uniform sampler2D diff_texs[16];\n"
        "void main() {\n"
        "   if (Mat.num_diffuse == 0) {\n"
        "      FragColour = vec4(Mat.colour, 1.0);\n"
        "   } else {\n"
        "      FragColour = texture(diff_texs[0], InterpMeshTexCoords);\n"
        "   }\n}",
        // HUD fragment 6
        "out vec4 FragColour;\n"
        "in vec2 TexCoord;\n"
        "flat in uint tex_index;\n"
        "uniform sampler2D diff_texs[16];\n"
        "void main() {\n"
        "   FragColour = texture(diff_texs[tex_index], TexCoord);\n"
        "   if (FragColour.w < 0.01) discard;\n}",
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
        "struct Material {\n"
        "   float spec_fac, spec_exp, opacity;\n"
        "   vec3 colour;\n"
        "   uint num_diffuse, num_specular;\n"
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
        "out vec4 FragColour;\n"
        "in vec2 InterpMeshTexCoords;\n"
        "in vec3 InterpMeshNormal;\n"
        "in vec3 FragPosition;\n"
		"flat in Material Mat;\n"
        "uniform vec3 camera_pos;\n"
		"uniform float ambience;\n"
        "uniform sampler2D diff_texs[16];\n"
        "uniform sampler2D spec_texs[16];\n"
        "void main() {\n"
        "    vec3 Normal = normalize(InterpMeshNormal) * (gl_FrontFacing ? 1.0f : -1.0f);\n"
        "    int i;\n"
        "    vec3 final_shade = vec3(ambience);\n"
        "    vec4 obj_base = (Mat.num_diffuse == 0 ? vec4(Mat.colour, 1.0) : texture(diff_texs[0], InterpMeshTexCoords));\n"
        "    if (obj_base.w * Mat.opacity < 0.001) discard;\n"
        "    vec3 spec_frag = (Mat.num_specular == 0 ? vec3(Mat.spec_fac) : texture(spec_texs[0], InterpMeshTexCoords).xyz);\n"
        "    for (i = 0; i < point_lights_size; i++) {\n"
        "        vec3 light_dir = point_lights[i].a.xyz - FragPosition;\n"
        "        float dist = max(0.001, length(light_dir));\n"
        "        float atten = 1.0 / (1.0 + point_lights[i].a.w * dist + point_lights[i].b.w * dist * dist);\n"
        "        light_dir /= dist;\n"
        "        final_shade += (\n"
        "            point_lights[i].b.xyz * max(0.0, dot(Normal, light_dir)) +\n"
        "            point_lights[i].b.xyz * spec_frag * pow(max(dot(Normal, normalize(normalize(camera_pos - FragPosition) + light_dir)), 0.0), Mat.spec_exp)\n"
        "        ) * atten;\n"
        "    }\n"
        "    for (i = 0; i < ray_lights_size; i++) {\n"
        "        vec3 colour = vec3(ray_lights[i].a.w, ray_lights[i].b);\n"
        "        final_shade += colour * max(0.0, dot(Normal, ray_lights[i].a.xyz)) + colour * spec_frag * pow(max(dot(Normal, normalize(normalize(camera_pos - FragPosition) + ray_lights[i].a.xyz)), 0.0), Mat.spec_exp);\n"
        "    }\n"
        "    for (i = 0; i < flash_lights_size; i++) {\n"
        "        vec3 light_dir = FragPosition - flash_lights[i].a.xyz;\n"
        "        float dist = max(0.001, length(light_dir));\n"
        "        float atten = 1.0 / (1.0 + flash_lights[i].a.w * dist + flash_lights[i].b.w * dist * dist);\n"
        "        light_dir /= dist;\n"
        "        float cone_factor = ((flash_lights[i].c.w - acos(dot(light_dir, flash_lights[i].b.xyz))) / 0.0349) + 1.0;\n"
        "        vec3 colour = flash_lights[i].c.xyz * max(0.0, min(1.0, cone_factor));\n"
        "        final_shade += (\n"
        "            colour * max(0.0, dot(Normal, -light_dir)) +\n"
        "            colour * spec_frag * pow(max(dot(Normal, normalize(normalize(camera_pos - FragPosition) - light_dir)), 0.0), Mat.spec_exp)\n"
        "        ) * atten;\n"
        "    }\n"
        "    FragColour = vec4(obj_base.xyz * final_shade, obj_base.w * Mat.opacity);\n}",
        // Skybox setup fragment 8
        "out vec4 FragColor;\n"
        "in vec3 TexCoords;\n"
        "uniform sampler2D cubemap;\n"
        "void main() {\n"
        "   vec3 d = normalize(TexCoords);\n"
        "   float u = atan(d.z, d.x) / (2.0 * 3.14159265359) + 0.5;\n"
        "   float v = acos(clamp(d.y, -1.0, 1.0)) / 3.14159265359;\n"
        "   FragColor = texture(cubemap, vec2(u, v));\n}"
    };

    /**
    * Pass one or multiple shader program sources (stored within a vector) to load them.
    * 
    * @param source(s) Source structs containing shader program component paths
    */
    TAGShaderManager();
    ~TAGShaderManager();

    /**
    * Add new shader program(s)
    * 
    * @param source(s) Source struct(s) containing shader program component paths
    */
    template<ProgramType::Concept T> void addShader(const T& source);
    template<ProgramType::Concept T> void addShader(const std::vector<T>& sources);
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
    std::unordered_map<std::string, Shader> shaders;

    template<OpenGLObjectType::Concept T> static GLuint compileShader(const std::string& source_code);
    static GLuint compileProgram(const std::vector<GLuint>& shader_ids);
    static void getSourceCodeFromFile(std::string& source_code, const std::string& path);
    static void getSourceCodeFromDefault(DefaultProgramType type, std::string& vertex, std::string& fragment);
    template<ShaderUniformType::Concept T> static GLenum getEnumType();
};

namespace ProgramType {
    struct Default : public TagType {
        std::string geometry_path;
        TAGShaderManager::DefaultProgramType type;
    };
};

#include "ShaderManagerClass.inl"
