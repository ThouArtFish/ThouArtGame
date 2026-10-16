#pragma once

#include <string>
#include <numeric>
#include <vector>
#include <deque>
#include <array>
#include <iostream>
#include <concepts>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "BaseStateClass.hpp"
#include "ShaderManagerClass.hpp"
#include "ResourceManagerClass.hpp"
#include "TextureLoaderClass.hpp"

/**
 * Stores a singular mesh, consisting of vertices, indices, textures and values that modify material properties.
 * Primarily used by the Model classes as the base mesh for multiple game world instances of the mesh.
 * Only stores the physical, base properties of a mesh, not in-game instances, so it is not recommended to use TAGMesh draw functions
 * directly as shaders need to have instances of meshes setup beforehand, which is done by TAGModel draw functions.
 */
class TAGMesh : public TAGBaseState::OpenGLContextChecker {
    friend class TAGModel;
	public:
		/**
		 * Default binding location in vertex array object
		 */
        constexpr static inline GLuint base_attrib = 0;
        constexpr static inline GLuint default_vao_mesh_binding_point = 0;
		constexpr static inline GLuint default_vao_instance_binding_point = 1;
		constexpr static inline GLuint defualt_vao_material_binding_point = 2;

        /**
        * BVH tree generation constants
        */
        constexpr static inline GLuint bvh_box_max_size = 4;
        constexpr static inline GLuint bvh_max_depth = 5;

        /**
         * Represents a single vertex of a mesh, including its local position, outward normal and texture coordinate.
         */
        struct Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 tex_coord;
        };
		constexpr static inline GLuint vertex_component_count = sizeof(Vertex) / sizeof(GLfloat);

        /**
         * Represents a single fragment, or primitive, composed of 3 vertex indices
         */
		constexpr static inline GLuint fragment_component_count = 3;
		using Fragment = std::array<GLuint, fragment_component_count>;

        /**
		* Links a group of fragments to a material
        */
        struct MaterialFragments {
            GLuint material_index = 0;
            std::vector<Fragment> frags;
        };

        /**
         * Container for various values that modify the material of a mesh.
         */
        struct Material {
            std::string name = "Default";
            float spec_fac = 0.0f;
            float spec_exp = 32.0f;
            float opacity = 1.0f;
            glm::vec3 colour = glm::vec3(1.0f, 0.0f, 0.0f);
            std::vector<TAGTexLoader::Texture> textures;

            TAGTexLoader::Texture& getTexture(const std::string& name);
        };
        /**
		* Shader equivalent of Material, used for passing values to shaders
        */
        struct alignas(4) ShaderMaterial {
			GLfloat spec_fac = 0.0f, spec_exp = 32.0f, opacity = 1.0f;
            glm::vec3 colour = glm::vec3(1.0f, 0.0f, 0.0f);
			GLuint num_diffuse = 0, num_specular = 0;
        };

        /**
        * Axis aligned bounding box
        */
        struct BoundingBox {
            glm::vec3 max, min;

            bool collisionPoint(const glm::vec3& point) const;
            bool collisionBBox(const BoundingBox& box) const;
            bool collisionRay(const glm::vec3& start, const glm::vec3& ray, const float& t = -1.0f) const;
            bool collisionSphere(const glm::vec3& centre, const float& radius) const;
            bool collisionCapsule(const glm::vec3& foot, const glm::vec3& spine, const float& radius) const;
        };
        /**
        * A quad tree box, indices could be for lower level quad tree boxes, or mesh planes, depending on is_leaf
        */
        struct BVHNode {
            bool is_leaf;
            BoundingBox bounds;
            std::vector<size_t> indices;
        };
        /**
        * Smaller plane struct that only represents the infinite plane a fragment lies in
        */
        struct DotPlane {
            glm::vec3 normal = {};
            float constant = 0.0f;
        };
        /**
        * Represents a mesh fragment in game space
        */
        struct Plane {
            glm::vec3 normal, start;
            std::array<glm::vec3, 2> axis;

            bool collisionBBox(const BoundingBox& box) const;
            bool collisionPoint(const glm::vec3& point) const;
            bool collisionRay(const glm::vec3& start, const glm::vec3& ray, const float& t = 1.0f) const;
        };
        /**
        * Represents the volume of a plane used for collision detection with spheres, and ray detection with frag plane.
        */
        struct PlaneVolume {
            Plane frag_plane;
            std::array<DotPlane, 3> volume_planes;

            DotPlane collisionSphere(const glm::vec3& centre, const float& radius) const;
            DotPlane collisionCapsule(const glm::vec3& foot, const glm::vec3& spine, const float& radius) const;
        };

        /**
         * Define a mesh from the parameters.
         * 
         * @param vertices All the vertices of the mesh.
		 * @param material_frags Groups of fragments that share the same material.
         * @param materials Mesh materials.
         */
        TAGMesh(const std::vector<Vertex>& vertices, const std::vector<MaterialFragments>& material_frags, const std::vector<Material>& materials);
        TAGMesh(const std::vector<Vertex>& vertices, const std::vector<Fragment>& frags, const Material& material);
        ~TAGMesh();

        /**
        * Getters for mesh data
        */
        GLuint getVBO() const;
        GLuint getEBO() const;
        GLuint getMaterialBuffer() const;
        BoundingBox getBoundingBox() const;
        const std::vector<PlaneVolume>& getPlanes() const;
        const std::vector<BVHNode>& getBVH() const;

        /**
        * Generate bounding box from array of vectors
        */
        static BoundingBox generateBoundingBox(const glm::vec3* start, const unsigned int& size);
	private:
        struct DrawCommand {
            size_t material_index, element_start, element_count;
        };
		using VertexBuffer = TAGResourceManager::ObjectBuffer<Vertex, GLfloat, vertex_component_count - 1>;
		using FragmentBuffer = TAGResourceManager::ObjectBuffer<Fragment, GLuint, fragment_component_count - 1>;
		using MaterialBuffer = TAGResourceManager::ObjectBuffer<Material, ShaderMaterial>;

		VertexBuffer VBO;
		FragmentBuffer EBO;
        MaterialBuffer material_buffer;
		std::vector<DrawCommand> draw_commands;
        BoundingBox mesh_bb;
        std::vector<PlaneVolume> planes;
        std::vector<BVHNode> bvh_octree;
        static inline GLuint VAO = 0;

        /**
         * Draw multiple instances of a mesh
         *
         * @param shader Shader program
         * @param options Names of shader uniforms
         * @param number Number of instances to draw
         */
        void draw(const TAGShaderManager::Shader& shader, const TAGShaderManager::ShaderOptions& options, const GLuint& number = 1);

        /**
		* Conversion functions for converting to shader compatible data
        */
		static std::array<GLfloat, vertex_component_count> convertVertex(const Vertex& vertex);
        static std::array<GLuint, fragment_component_count> convertFragment(const Fragment& frag);
		static std::array<ShaderMaterial, 1> convertMaterial(const Material& material);

        /**
        * Setup functions
        */
        void setupMeshBoundingBox();
        static void setupMeshVertexArrayObject();
        /**
        * Generates game space planes to represent primitives for mesh
        */
        void generatePlanes();
        /**
        * Generates Bounding Volume Heirarchy for mesh, implemented with an octree.
        * Requires planes to be generated with generatePlanes first or does nothing.
        */
        void generateBVH();
};
