#pragma once

#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <concepts>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include "ShaderManagerClass.hpp"
#include "ResourceManagerClass.hpp"
#include "TextureLoaderClass.hpp"

/**
 * Stores a singular mesh, consisting of vertices, indices, textures and values that modify material properties.
 * Primarily used by the Model classes as the base mesh for multiple game world instances of the mesh.
 * Only stores the physical, base properties of a mesh, not in-game instances, so it is not recommended to use TAGMesh draw functions
 * directly as shaders need to have instances of meshes setup beforehand, which is done by TAGModel draw functions.
 */
class TAGMesh {
    friend class TAGModel;
	public:
        /**
         * Represents a single vertex of a mesh, including its local position, outward normal and texture coordinate.
         */
        struct Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 tex_coord;
        };

        /**
         * Represents a single fragment, or primitive, composed of 3 vertex indices and a material index.
         */
        struct Fragment {
            std::array<unsigned int, 3> vertex_indices;
            unsigned int material_index = 0;
        };

        /**
        * Element buffer object, containing fragments for a specific material
        */
        struct MaterialElementBuffer {
            unsigned int EBO = 0;
            unsigned int material_index = 0;
            ~MaterialElementBuffer();
        };

        /**
         * Container for various values that modify the material of a mesh.
         */
        struct Material {
            std::string name = "Default";
            float spec_mod = 0.0f;
            float spec_exp = 32.0f;
            float opacity = 1.0f;
            glm::vec3 colour = glm::vec3(1.0f, 0.0f, 0.0f);
            std::vector<TAGTexLoader::Texture> textures;

            TAGTexLoader::Texture& getTexture(const std::string& name);
        };

        /**
        * Axis aligned bounding box
        */
        struct BoundingBox {
            glm::vec3 max;
            glm::vec3 min;
        };

        /**
        * A quad tree box, indices could be for lower level quad tree boxes, or mesh planes, depending on is_leaf
        */
        struct BVHNode {
            bool is_leaf;
            BoundingBox bounds;
            std::vector<unsigned int> indices;
        };

        /**
        * Represents a mesh fragment in game space
        */
        struct Plane {
            glm::vec3 normal;
            glm::vec3 start;
            std::array<glm::vec3, 2> axis;
        };
        /**
         * Smaller plane struct that only represents the infinite plane a fragment lies in
         */
        struct DotPlane {
            glm::vec3 normal;
            float constant;
        };
        /**
         * Represents the volume of a plane used for collision detection with spheres, and ray detection with frag plane.
         */
        struct PlaneVolume {
            Plane frag_plane;
            std::array<DotPlane, 3> volume_planes;
        };

        bool delete_on_death = true;
        static inline unsigned int base_attrib = 0;
        BoundingBox mesh_bb;
        std::vector<PlaneVolume> planes;
        std::vector<BVHNode> bvh_octree;

        /**
         * Define a mesh from the parameters.
         * 
         * @param vertices All the vertices of the mesh.
         * @param indices Indices of vertices for drawing primitives. Each sequence of 3 indices represents one primitive.
         * @param textures Textures used on mesh.
         * @param materials Mesh materials.
         */
        TAGMesh(const std::vector<Vertex>& vertices, const std::vector<Fragment>& frags, const std::vector<Material>& materials);
        TAGMesh();
        ~TAGMesh();
        /**
        * Generates game space planes to represent primitives for mesh
        */
        void generatePlanes();
        /**
        * Generates Bounding Volume Heirarchy for mesh, implemented with an octree.
        * Requires planes to be generated with generatePlanes first or does nothing.
        */
        void generateBVH();
        /**
         * Setups up a vertex array object that allows quick assigning of the physical mesh for drawing.
         */
        void setupMesh();
        /**
         * Get vector of vertices.
         */
        std::vector<Vertex>& changeVertices();
        const std::vector<Vertex>& getVertices() const;
        /**
         * Get vector of fragments.
         */
        std::vector<Fragment>& changeFragments();
        const std::vector<Fragment>& getFragments() const;
        /**
        * Get a material at an index
        * 
        * @param index Index of material
        */
        Material& getMaterial(const unsigned int& index);
        Material& getMaterial(const std::string& name);
        /**
         * Return vertex array object ID
         */
        const unsigned int& getVAO() const;
        /**
         * Return element buffer object IDs
         */
        const std::vector<MaterialElementBuffer>& getEBOs() const;
        /**
         * Return vertex buffer object ID
         */
        const unsigned int& getVBO() const;

        /**
        * Collision detection functions
        */
        static bool FragWithPoint(const glm::vec3& point, const Plane& plane);
        static bool FragWithSphere(const glm::vec3& centre, const float& radius, const PlaneVolume& plane);
        static bool FragWithCapsule(const glm::vec3& foot, const glm::vec3& spine, const float& radius, const PlaneVolume& plane);
        static bool BBoxWithBBox(const BoundingBox& box_a, const BoundingBox& box_b);
        static bool BBoxWithRay(const BoundingBox& box, const glm::vec3& start, const glm::vec3& ray, const float& factor);
        static bool BBoxWithCapsule(const BoundingBox& box, const glm::vec3& foot, const glm::vec3& spine, const float& radius);
        static bool BBoxWithSphere(const BoundingBox& box, const glm::vec3& centre, const float& radius);

        /**
        * Generate bounding box from array of vectors
        */
        static BoundingBox generateBoundingBox(const glm::vec3* start, const unsigned int& size);
	private:
        static constexpr inline unsigned int bvh_box_max_size = 4;
		std::vector<Vertex> vertices;
		std::vector<Fragment> frags;
        std::vector<Material> materials;
        std::vector<MaterialElementBuffer> material_ebos;
        bool vertices_updated = false;
        bool frags_updated = false;
        bool is_transparent = false;
        unsigned int VAO = 0;
        unsigned int VBO = 0;

        void applyBufferUpdates();
        /**
         * Setup mesh-related shader uniforms, such as textures and material modifier values.
         * Not sufficient to draw a singular instance of a mesh.
         *
         * @param shader Shader program
         */
        void setupFragmentUniforms(const TAGShaderManager::Shader& shader, const unsigned int& material_index) const;
        /**
         * Draw multiple instances of a mesh
         *
         * @param shader Shader program
         * @param number Number of instances
         */
        void drawInstanced(const TAGShaderManager::Shader& shader, const unsigned int& number);
        /**
        * Draw one instance of a mesh.
        *
        * @param shader Shader program
        */
        void drawUninstanced(const TAGShaderManager::Shader& shader);
};
