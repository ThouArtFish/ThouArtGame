#pragma once

#include <WorldModelClass.hpp>

template<Collision::RayScope T> T TAGWorldModel::rayCollision(const glm::vec3& start, const glm::vec3& ray_dir, const float& max) const {
	ret.emplace<T>(T());
	t = -1.0;
	Collision::Info info;

	for (const auto& instance_pair : instances) {
		if (!instance_pair.second.empty()) {
			if (instance_pair.first == "") {
				for (const auto& mesh_pair : meshes) {
					info = rayCollisionWithMeshInstances<T>(start, ray_dir, max, mesh_pair.second, instance_pair.second);
					if constexpr (std::same_as<T, Collision::ANY>) {
						if (info.index >= 0) {
							return info;
						}
					}
				}
			}
			else {
				info = rayCollisionWithMeshInstances<T>(start, ray_dir, max, meshes.at(instance_pair.first), instance_pair.second);
				if constexpr (std::same_as<T, Collision::ANY>) {
					if (info.index >= 0) {
						return info;
					}
				}
			}
		}
	}
	return std::get<T>(ret);
}

template<Collision::RayScope T> Collision::STATIC<T>::type TAGWorldModel::rayCollision(const glm::vec3& start, const glm::vec3& ray_dir, std::vector<Collision::Info>& collisions, const float& max) {
	ret.emplace<Collision::STATIC<T>::type>(Collision::STATIC<T>::type());
	if constexpr (!std::same_as<Collision::ALL, T>) {
		ret = -1;
	}
	t = -1.0;

	for (size_t i = 0; i < collisions.size(); i++) {
		const TAGMesh::Plane& plane = collisions[i].plane.frag_plane;
		double d = glm::dot(plane.normal, ray_dir);
		if (glm::abs(d) < 0.0001) {
			continue;
		}
		d = glm::dot(plane.normal, plane.start - start) / d;
		if (d >= 0.0 && (max < 0.0f || d <= max)) {
			bool cont = true;
			if constexpr (std::same_as<T, Collision::FURTHEST>) {
				cont = (d > t);
			}
			else if constexpr (std::same_as<T, Collision::CLOSEST>) {
				cont = (t < 0.0 || d < t);
			}
			if (cont && TAGMesh::FragWithPoint(ray_dir * (float)d + start, plane)) {
				if constexpr (std::same_as<T, Collision::ANY>) {
					return (int)i;
				}
				else if constexpr (std::same_as<T, Collision::ALL>) {
					std::get<Collision::STATIC<T>::type>(ret).push_back(i);
				}
				else {
					ret.emplace<Collision::STATIC<T>::type>((int)i);
					t = d;
				}
			}
		}
	}
	return std::get<Collision::STATIC<T>::type>(ret);
}

template<Collision::ColliderScope T> T TAGWorldModel::capsuleCollision(const glm::vec3& foot, const glm::vec3& spine, const float& radius) const {
	ret.emplace<T>(T());
	Collision::Info info;

	for (const auto& instance_pair : instances) {
		if (!instance_pair.second.empty()) {
			if (instance_pair.first == "") {
				for (const auto& mesh_pair : meshes) {
					info = capsuleCollisionMeshInstances<T>(foot, spine, radius, mesh_pair.second, instance_pair.second);
					if constexpr (std::same_as<T, Collision::ANY>) {
						if (info.index >= 0) {
							return info;
						}
					}
				}
			}
			else {
				info = capsuleCollisionMeshInstances<T>(foot, spine, radius, meshes.at(instance_pair.first), instance_pair.second);
				if constexpr (std::same_as<T, Collision::ANY>) {
					if (info.index >= 0) {
						return info;
					}
				}
			}
		}
	}
	return std::get<T>(ret);
}

template<Collision::ColliderScope T> T TAGWorldModel::sphereCollision(const glm::vec3& centre, const float& radius) const {
	ret.emplace<T>(T());
	Collision::Info info;

	for (const auto& instance_pair : instances) {
		if (!instance_pair.second.empty()) {
			if (instance_pair.first == "") {
				for (const auto& mesh_pair : meshes) {
					info = sphereCollisionWithMeshInstances<T>(centre, radius, mesh_pair.second, instance_pair.second);
					if constexpr (std::same_as<T, Collision::ANY>) {
						if (info.index >= 0) {
							return info;
						}
					}
				}
			}
			else {
				info = sphereCollisionWithMeshInstances<T>(centre, radius, meshes.at(instance_pair.first), instance_pair.second);
				if constexpr (std::same_as<T, Collision::ANY>) {
					if (info.index >= 0) {
						return info;
					}
				}
			}
		}
	}
	return std::get<T>(ret);
}

template<Collision::ColliderScope T> Collision::STATIC<T>::type TAGWorldModel::sphereCollision(const glm::vec3& centre, const float& radius, std::vector<Collision::Info>& collisions) {
	ret.emplace<Collision::STATIC<T>::type>(Collision::STATIC<T>::type());
	if constexpr (std::same_as<Collision::ANY, T>) {
		ret = -1;
	}
	
	for (size_t i = 0; i < collisions.size(); i++) {
		if (TAGMesh::FragWithSphere(centre, radius, collisions[i].plane)) {
			if constexpr (std::same_as<Collision::ALL, T>) {
				std::get<Collision::STATIC<T>::type>(ret).push_back(i);
			}
			else {
				return (int)i;
			}
		}
	}
	return std::get<Collision::STATIC<T>::type>(ret);
}

template<Collision::RayScope T> Collision::Info TAGWorldModel::rayCollisionWithMeshInstances(const glm::vec3& start, const glm::vec3& ray_dir, const float& max, const TAGMesh& mesh, const std::vector<Object>& objs) {
	const std::vector<TAGMesh::BVHNode>& bvh = mesh.getBVH();
	const std::vector<TAGMesh::PlaneVolume>& planes = mesh.getPlanes();

	for (const Object& obj : objs) {
		indices.clear();
		octree_stack.push_back(0);

		const float local_scale = 1.0f / obj.scale;
		const glm::mat3 inverse_rot = glm::transpose(glm::mat3(glm::rotate(glm::mat4(1.0f), obj.angle, obj.rotation_axis)));
		const glm::vec3 local_start = inverse_rot * (start - obj.position) * local_scale;
		const glm::vec3 local_ray = inverse_rot * ray_dir * local_scale;

		while (!octree_stack.empty()) {
			const TAGMesh::BVHNode& current_box = bvh[octree_stack.back()];
			octree_stack.pop_back();

			if (TAGMesh::BBoxWithRay(current_box.bounds, local_start, local_ray, max)) {
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
			const TAGMesh::Plane& plane = planes[plane_index].frag_plane;
			double d = glm::dot(plane.normal, local_ray);
			if (glm::abs(d) < 0.0001) {
				continue;
			}
			d = glm::dot(plane.normal, plane.start - local_start) / d;
			if (d >= 0.0 && (max < 0.0f || d <= max)) {
				bool cont = true;
				if constexpr (std::same_as<T, Collision::FURTHEST>) {
					cont = (d > t);
				}
				else if constexpr (std::same_as<T, Collision::CLOSEST>) {
					cont = (t < 0.0 || d < t);
				}
				if (cont && TAGMesh::FragWithPoint(ray_dir * (float)d + start, plane)) {
					if constexpr (std::same_as<T, Collision::ANY>) {
						return { planeToGameSpace(planes[plane_index], obj), plane_index };
					}
					else if constexpr (std::same_as<T, Collision::ALL>) {
						std::get<T>(ret).emplace_back(planeToGameSpace(planes[plane_index], obj), plane_index);
					}
					else {
						ret = { planeToGameSpace(planes[plane_index], obj), plane_index };
						t = d;
					}
				}
			}
		}
	}
	return Collision::Info();
}

template<Collision::ColliderScope T> Collision::Info TAGWorldModel::capsuleCollisionMeshInstances(const glm::vec3& foot, const glm::vec3& spine, const float& radius, const TAGMesh& mesh, const std::vector<Object>& objs) {
	const std::vector<TAGMesh::BVHNode>& bvh = mesh.getBVH();
	const std::vector<TAGMesh::PlaneVolume>& planes = mesh.getPlanes();

	for (const Object& obj : objs) {
		indices.clear();
		octree_stack.push_back(0);

		const float local_scale = 1.0f / obj.scale;
		const glm::mat3 inverse_rot = glm::transpose(glm::mat3(glm::rotate(glm::mat4(1.0f), obj.angle, obj.rotation_axis)));
		const glm::vec3 local_foot = inverse_rot * (foot - obj.position) * local_scale;
		const glm::vec3 local_spine = inverse_rot * spine * local_scale;
		const float local_radius = radius * local_scale;

		while (!octree_stack.empty()) {
			const TAGMesh::BVHNode& current_box = bvh[octree_stack.back()];
			octree_stack.pop_back();

			if (TAGMesh::BBoxWithCapsule(current_box.bounds, local_foot, local_spine, local_radius)) {
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
			if (TAGMesh::FragWithCapsule(local_foot, local_spine, local_radius, planes[plane_index])) {
				if constexpr (std::same_as<T, Collision::ALL>) {
					std::get<T>(ret).emplace_back(planeToGameSpace(planes[plane_index], obj), plane_index);
				}
				else {
					return { planeToGameSpace(planes[plane_index], obj), plane_index };
				}
			}
		}
	}
	return Collision::Info();
}

template<Collision::ColliderScope T> Collision::Info TAGWorldModel::sphereCollisionWithMeshInstances(const glm::vec3& centre, const float& radius, const TAGMesh& mesh, const std::vector<Object>& objs) {
	const std::vector<TAGMesh::BVHNode>& bvh = mesh.getBVH();
	const std::vector<TAGMesh::PlaneVolume>& planes = mesh.getPlanes();

	for (const Object& obj : objs) {
		indices.clear();
		octree_stack.push_back(0);

		const float local_scale = 1.0f / obj.scale;
		const glm::vec3 local_centre = glm::transpose(glm::mat3(glm::rotate(glm::mat4(1.0f), obj.angle, obj.rotation_axis))) * (centre - obj.position) * local_scale;
		const float local_radius = radius * local_scale;

		while (!octree_stack.empty()) {
			const TAGMesh::BVHNode& current_box = bvh[octree_stack.back()];
			octree_stack.pop_back();

			if (TAGMesh::BBoxWithSphere(current_box.bounds, local_centre, local_radius)) {
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
			if (TAGMesh::FragWithSphere(local_centre, local_radius, planes[plane_index])) {
				if constexpr (std::same_as<Collision::ALL, T>) {
					std::get<T>(ret).emplace_back(planeToGameSpace(planes[plane_index], obj), plane_index);
				}
				else {
					return { planeToGameSpace(planes[plane_index], obj), plane_index };
				}
			}
		}
	}
	return Collision::Info();
}
