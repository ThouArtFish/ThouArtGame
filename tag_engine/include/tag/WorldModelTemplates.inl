#pragma once

#include <WorldModelClass.hpp>

template<Collision::RayScope T> T TAGWorldModel::rayCollision(const glm::vec3& start, const glm::vec3& ray_dir, const float& max) const {
	Ret ret;
	Collision::Info info;

	ret.emplace<T>(T());

	for (const auto& instance_pair : instance_buffers) {
		const auto& instances = instance_pair.second.getAllObjects();
		if (!instance_pair.second.empty()) {
			if (instance_pair.first == "") {
				for (const auto& mesh_pair : meshes) {
					const T res = rayCollisionWithMeshInstances<T>(start, ray_dir, max, mesh_pair.second, instances);
					if constexpr (std::same_as<T, Collision::ANY>) {
						if (res.index >= 0) return res;
					}
					else if constexpr (std::same_as<T, Collision::ALL>) {
						auto& vec = std::get<T>(ret);
						vec.insert(vec.end(), res.begin(), res.end());
					}
					else {
						ret = res;
					}
				}
			}
			else {
				const T res = rayCollisionWithMeshInstances<T>(start, ray_dir, max, meshes.at(instance_pair.first), instances);
				if constexpr (std::same_as<T, Collision::ANY>) {
					if (res.index >= 0) return res;
				}
				else if constexpr (std::same_as<T, Collision::ALL>) {
					auto& vec = std::get<T>(ret);
					vec.insert(vec.end(), res.begin(), res.end());
				}
				else {
					if (ret.index < 0) ret = res;

					else {
						int i = rayCollision<T>(start, ray_dir, { std::get<T>(ret), res }, max);

						if (i > 0) ret = res;
					} 
				}
			}
		}
	}
	return std::get<T>(ret);
}

template<Collision::RayScope T> Collision::STATIC<T>::TYPE TAGWorldModel::rayCollision(const glm::vec3& start, const glm::vec3& ray_dir, const std::vector<Collision::Info>& collisions, const float& max) {
	Ret ret;
	double t = -1.0;

	ret.emplace<Collision::STATIC<T>::TYPE>(Collision::STATIC<T>::TYPE());

	if constexpr (!std::same_as<Collision::ALL, T>) ret = -1;

	for (size_t i = 0; i < collisions.size(); i++) {
		const TAGMesh::Plane& plane = collisions[i].plane.frag_plane;
		float d = glm::dot(plane.normal, ray_dir);

		if (glm::abs(d) < 0.0001) continue;

		d = glm::dot(plane.normal, plane.start - start) / d;
		if (d >= 0.0 && (max < 0.0f || d <= max)) {
			bool cont = true;

			if constexpr (std::same_as<T, Collision::FURTHEST>) cont = (d > t);

			else if constexpr (std::same_as<T, Collision::CLOSEST>) cont = (t < 0.0 || d < t);

			if (cont && plane.collisionPoint(ray_dir * d + start)) {

				if constexpr (std::same_as<T, Collision::ANY>) return (int)i;

				else if constexpr (std::same_as<T, Collision::ALL>) std::get<Collision::STATIC<T>::TYPE>(ret).push_back(i);

				else {
					ret = i;
					t = d;
				}
			}
		}
	}
	return std::get<Collision::STATIC<T>::TYPE>(ret);
}

template<Collision::ColliderScope T> T TAGWorldModel::capsuleCollision(const glm::vec3& foot, const glm::vec3& spine, const float& radius) const {
	Ret ret;

	ret.emplace<T>(T());

	for (const auto& instance_pair : instance_buffers) {
		const auto& instances = instance_pair.second.getAllObjects();
		if (!instances.empty()) {
			if (instance_pair.first == "") {
				for (const auto& mesh_pair : meshes) {
					T res = capsuleCollisionMeshInstances<T>(foot, spine, radius, mesh_pair.second, instances);
					if constexpr (std::same_as<T, Collision::ANY>) {
						if (res.index >= 0) return res;
					}
					else {
						auto& vec = std::get<T>(ret);
						vec.insert(vec.end(), res.begin(), res.end());
					}
				}
			}
			else {
				T res = capsuleCollisionMeshInstances<T>(foot, spine, radius, meshes.at(instance_pair.first), instances);
				if constexpr (std::same_as<T, Collision::ANY>) {
					if (res.index >= 0) return res;
				}
				else {
					auto& vec = std::get<T>(ret);
					vec.insert(vec.end(), res.begin(), res.end());
				}
			}
		}
	}
	return std::get<T>(ret);
}

template<Collision::ColliderScope T> static Collision::STATIC<T>::TYPE TAGWorldModel::capsuleCollision(const glm::vec3& foot, const glm::vec3& spine, const float& radius, const std::vector<Collision::Info>& collisions) {
	Ret ret;

	ret.emplace<Collision::STATIC<T>::TYPE>(Collision::STATIC<T>::TYPE());

	if constexpr (std::same_as<Collision::ANY, T>) ret = -1;

	for (size_t i = 0; i < collisions.size(); i++) {
		if (collisions[i].plane.collisionCapsule(foot, spine, radius)) {
			if constexpr (std::same_as<Collision::ALL, T>) {
				std::get<Collision::STATIC<T>::TYPE>(ret).push_back(i);
			}
			else {
				return (int)i;
			}
		}
	}
	return std::get<Collision::STATIC<T>::TYPE>(ret);
}

template<Collision::ColliderScope T> T TAGWorldModel::sphereCollision(const glm::vec3& centre, const float& radius) const {
	Ret ret;

	ret.emplace<T>(T());

	for (const auto& instance_pair : instance_buffers) {
		const auto& instances = instance_pair.second.getAllObjects();
		if (!instances.empty()) {
			if (instance_pair.first == "") {
				for (const auto& mesh_pair : meshes) {
					T res = sphereCollisionWithMeshInstances<T>(centre, radius, mesh_pair.second, instances);
					if constexpr (std::same_as<T, Collision::ANY>) {
						if (res.index >= 0) return res;
					}
					else {
						auto& vec = std::get<T>(ret);
						vec.insert(vec.end(), res.begin(), res.end());
					}
				}
			}
			else {
				T res = sphereCollisionWithMeshInstances<T>(centre, radius, meshes.at(instance_pair.first), instances);
				if constexpr (std::same_as<T, Collision::ANY>) {
					if (res.index >= 0) return res;
				}
				else {
					auto& vec = std::get<T>(ret);
					vec.insert(vec.end(), res.begin(), res.end());
				}
			}
		}
	}
	return std::get<T>(ret);
}

template<Collision::ColliderScope T> Collision::STATIC<T>::TYPE TAGWorldModel::sphereCollision(const glm::vec3& centre, const float& radius, const std::vector<Collision::Info>& collisions) {
	Ret ret;

	ret.emplace<Collision::STATIC<T>::TYPE>(Collision::STATIC<T>::TYPE());

	if constexpr (std::same_as<Collision::ANY, T>) ret = -1;
	
	for (size_t i = 0; i < collisions.size(); i++) {
		if (collisions[i].plane.collisionSphere(centre, radius)) {
			if constexpr (std::same_as<Collision::ALL, T>) {
				std::get<Collision::STATIC<T>::TYPE>(ret).push_back(i);
			}
			else {
				return (int)i;
			}
		}
	}
	return std::get<Collision::STATIC<T>::TYPE>(ret);
}

template<Collision::RayScope T> T TAGWorldModel::rayCollisionWithMeshInstances(const glm::vec3& start, const glm::vec3& ray_dir, const float& max, const TAGMesh& mesh, const std::vector<Object>& objs) {
	Ret ret;
	vui octree_stack, indices;
	double t = -1.0;

	ret.emplace<T>(T());

	for (const Object& obj : objs) {
		indices.clear();
		octree_stack.push_back(0);

		const float local_scale = 1.0f / obj.scale;
		const glm::mat3 inverse_rot = glm::transpose(glm::mat3(glm::rotate(glm::mat4(1.0f), obj.angle, obj.rotation_axis)));
		const glm::vec3 local_start = inverse_rot * (start - obj.position) * local_scale;
		const glm::vec3 local_ray = inverse_rot * ray_dir * local_scale;

		while (!octree_stack.empty()) {
			const TAGMesh::BVHNode& current_box = mesh.bvh_octree[octree_stack.back()];
			octree_stack.pop_back();

			if (current_box.bounds.collisionRay(local_start, local_ray, max)) {
				if (!current_box.is_leaf) {
					octree_stack.insert(octree_stack.end(), current_box.indices.begin(), current_box.indices.end());
				}
				else {
					for (const ui& plane_index : current_box.indices) {
						if (std::find(indices.begin(), indices.end(), plane_index) == indices.end()) {
							indices.push_back(plane_index);
						}
					}
				}
			}
		}

		for (const ui& plane_index : indices) {
			const TAGMesh::Plane& plane = mesh.planes[plane_index].frag_plane;

			float d = glm::dot(plane.normal, local_ray);

			if (glm::abs(d) < 0.0001) continue;

			d = glm::dot(plane.normal, plane.start - local_start) / d;
			if (d >= 0.0f && (max < 0.0f || d <= max)) {
				bool cont = true;

				if constexpr (std::same_as<T, Collision::FURTHEST>) cont = (d > t);

				else if constexpr (std::same_as<T, Collision::CLOSEST>) cont = (t < 0.0f || d < t);

				if (cont && plane.collisionPoint(ray_dir * d + start)) {
					if constexpr (std::same_as<T, Collision::ANY>) return Collision::Info(planeToGameSpace(mesh.planes[plane_index], obj), plane_index);

					else if constexpr (std::same_as<T, Collision::ALL>) std::get<T>(ret).emplace_back(planeToGameSpace(mesh.planes[plane_index], obj), plane_index);

					else {
						ret = { planeToGameSpace(mesh.planes[plane_index], obj), plane_index };
						t = d;
					}
				}
			}
		}
	}
	return std::get<T>(ret);
}

template<Collision::ColliderScope T> T TAGWorldModel::capsuleCollisionMeshInstances(const glm::vec3& foot, const glm::vec3& spine, const float& radius, const TAGMesh& mesh, const std::vector<Object>& objs) {
	Ret ret;
	vui indices, octree_stack;
	
	ret.emplace<T>(T());

	for (const Object& obj : objs) {
		indices.clear();
		octree_stack.push_back(0);

		const float local_scale = 1.0f / obj.scale;
		const glm::mat3 inverse_rot = glm::transpose(glm::mat3(glm::rotate(glm::mat4(1.0f), obj.angle, obj.rotation_axis)));
		const glm::vec3 local_foot = inverse_rot * (foot - obj.position) * local_scale;
		const glm::vec3 local_spine = inverse_rot * spine * local_scale;
		const float local_radius = radius * local_scale;

		while (!octree_stack.empty()) {
			const TAGMesh::BVHNode& current_box = mesh.bvh_octree[octree_stack.back()];
			octree_stack.pop_back();

			if (current_box.bounds.collisionCapsule(local_foot, local_spine, local_radius)) {
				if (!current_box.is_leaf) {
					octree_stack.insert(octree_stack.end(), current_box.indices.begin(), current_box.indices.end());
				}
				else {
					for (const ui& plane_index : current_box.indices) {
						if (std::find(indices.begin(), indices.end(), plane_index) == indices.end()) {
							indices.push_back(plane_index);
						}
					}
				}
			}
		}

		for (const ui& plane_index : indices) {
			if (mesh.planes[plane_index].collisionCapsule(local_foot, local_spine, local_radius)) {
				if constexpr (std::same_as<T, Collision::ALL>) {
					std::get<T>(ret).emplace_back(planeToGameSpace(mesh.planes[plane_index], obj), plane_index);
				}
				else {
					return Collision::Info(planeToGameSpace(mesh.planes[plane_index], obj), plane_index);
				}
			}
		}
	}
	return std::get<T>(ret);
}

template<Collision::ColliderScope T> T TAGWorldModel::sphereCollisionWithMeshInstances(const glm::vec3& centre, const float& radius, const TAGMesh& mesh, const std::vector<Object>& objs) {
	Ret ret;
	vui indices, octree_stack;

	ret.emplace<T>(T());
	
	for (const Object& obj : objs) {
		indices.clear();
		octree_stack.push_back(0);

		const float local_scale = 1.0f / obj.scale;
		const glm::vec3 local_centre = glm::transpose(glm::mat3(glm::rotate(glm::mat4(1.0f), obj.angle, obj.rotation_axis))) * (centre - obj.position) * local_scale;
		const float local_radius = radius * local_scale;

		while (!octree_stack.empty()) {
			const TAGMesh::BVHNode& current_box = mesh.bvh_octree[octree_stack.back()];
			octree_stack.pop_back();

			if (current_box.bounds.collisionSphere(local_centre, local_radius)) {
				if (!current_box.is_leaf) {
					octree_stack.insert(octree_stack.end(), current_box.indices.begin(), current_box.indices.end());
				}
				else {
					for (const unsigned int& plane_index : current_box.indices) {
						if (std::find(indices.begin(), indices.end(), plane_index) == indices.end()) {
							indices.push_back(plane_index);
						}
					}
				}
			}
		}

		for (const ui& plane_index : indices) {
			if (mesh.planes[plane_index].collisionSphere(local_centre, local_radius)) {
				if constexpr (std::same_as<Collision::ALL, T>) {
					std::get<T>(ret).emplace_back(planeToGameSpace(mesh.planes[plane_index], obj), plane_index);
				}
				else {
					return Collision::Info(planeToGameSpace(mesh.planes[plane_index], obj), plane_index);
				}
			}
		}
	}
	return std::get<T>(ret);
}
