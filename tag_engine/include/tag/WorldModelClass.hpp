#pragma once

#include <array>
#include <string>
#include <vector>
#include <concepts>
#include <variant>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include "ModelClass.hpp"
#include "TextureLoaderClass.hpp"
#include "MeshClass.hpp"

/**
* Encapsulates various aspects of collision
*/
namespace Collision {
	/**
	* Collision info for a single plane, negative index means no collision at all
	*/
	struct Info {
		TAGMesh::PlaneVolume plane;
		int index = -1;
	};

	/**
	* Scope of collision detection and their corresponding return type
	*/
	typedef std::vector<Info> ALL;
	typedef	Info ANY;
	typedef Info FURTHEST;
	typedef Info CLOSEST;

	/**
	* Constraints for 3D collider types
	*/
	template<typename T> concept ColliderScope = std::same_as<T, ALL> || std::same_as<T, ANY>;
	/**
	* Constraints for Ray collider types
	*/
	template<typename T> concept RayScope = ColliderScope<T> || std::same_as<T, FURTHEST> || std::same_as<T, CLOSEST>;

	/**
	* Converts the collision detection scopes to their local versions
	*/
	template<RayScope T> struct STATIC {
		using type = int;
	};
	template<> struct STATIC<ALL> {
		using type = std::vector<unsigned int>;
	};
};

/**
* Class derived from TAGModel class for detecting collisions for certain collider types
*/
class TAGWorldModel : public TAGModel {
public:
	/**
	 * Path to the model file, and parameters for texture loading.
	 * Path can be empty, so TAGMesh can be added later.
	 *
	 * @param params Parameters for texture loading.
	 * @param path Path to model file.
	 */
	TAGWorldModel(const TAGTexLoader::Params& tex_params, const std::string& path = "");
	/**
	* Detects a collision between a sphere and each instance of every mesh.
	* 
	* @param centre Centre of sphere
	* @param radius Radius of sphere
	*/
	template<Collision::ColliderScope T> T sphereCollision(const glm::vec3& centre, const float& radius) const;
	/**
	* Detects a collision between a sphere and each provided Collision::Info struct.
	*
	* @param centre Centre of sphere
	* @param radius Radius of sphere
	* @param collisions Collision::Info structs from earlier collision tests
	*/
	template<Collision::ColliderScope T> static Collision::STATIC<T>::type sphereCollision(const glm::vec3& centre, const float& radius, std::vector<Collision::Info>& collisions);
	/**
	* Detects a collision between a capsule, a sphere that has been "swept" along a line, and each instance of every mesh.
	* 
	* @param foot The centre of the starting sphere in the capsule
	* @param spine Displacement vector from centre of starting sphere (foot) to the centre of the last sphere
	* @param radius Radius of capsule
	*/
	template<Collision::ColliderScope T> T capsuleCollision(const glm::vec3& foot, const glm::vec3& spine, const float& radius) const;
	/**
	* Detects a collision between a ray and each instance of every mesh.
	* 
	* @param start The position vector of the start of the ray
	* @param ray_dir The displacement vector from the start of the ray to the end.
	* @param max Defines the final point of the ray by [start + (max)(ray_dir)]. If max < 0 then ray is assumed to be infinite in length.
	*/
	template<Collision::RayScope T> T rayCollision(const glm::vec3& start, const glm::vec3& ray_dir, const float& max = 1.0f) const;
	/**
	* Detects a collision between a ray and each provided Collision::Info struct.
	*
	* @param start The position vector of the start of the ray
	* @param ray_dir The displacement vector from the start of the ray to the end.
	* @param collisions Collision::Info structs from earlier collision tests.
	* @param max Defines the final point of the ray by [start + (max)(ray_dir)]. If max < 0 then ray is assumed to be infinite in length.
	*/
	template<Collision::RayScope T> static Collision::STATIC<T>::type rayCollision(const glm::vec3& start, const glm::vec3& ray_dir, std::vector<Collision::Info>& collisions, const float& max = 1.0f);
private:
	using ui = unsigned int;
	using vui = std::vector<ui>;
	using Ret = std::variant<std::vector<Collision::Info>, Collision::Info, vui, int>;
	static inline vui indices;
	static inline vui octree_stack;
	static inline Ret ret;
	static inline double t = -1.0;

	static TAGMesh::PlaneVolume planeToGameSpace(const TAGMesh::PlaneVolume& plane, const Object& obj);
	template<Collision::RayScope T> static Collision::Info rayCollisionWithMeshInstances(const glm::vec3& start, const glm::vec3& ray_dir, const float& max, const TAGMesh& mesh, const std::vector<Object>& objs);
	template<Collision::ColliderScope T> static Collision::Info capsuleCollisionMeshInstances(const glm::vec3& foot, const glm::vec3& spine, const float& radius, const TAGMesh& mesh, const std::vector<Object>& objs);
	template<Collision::ColliderScope T> static Collision::Info sphereCollisionWithMeshInstances(const glm::vec3& centre, const float& radius, const TAGMesh& mesh, const std::vector<Object>& objs);
};

#include "../../src/WorldModelTemplates.inl"