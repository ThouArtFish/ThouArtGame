#include <MeshClass.hpp>

TAGMesh::TAGMesh(const std::vector<Vertex>& vertices, const std::vector<MaterialFragments>& material_frags, const std::vector<Material>& materials) :
	VBO(vertices.size(), convertVertex, TAGResourceManager::BufferAccess::STATIC),
	EBO(std::accumulate(material_frags.begin(), material_frags.end(), (size_t)0, [](size_t sum, const MaterialFragments& frag_struct) { return sum + frag_struct.frags.size(); }), convertFragment, TAGResourceManager::BufferAccess::STATIC),
	material_buffer(materials.size(), convertMaterial, TAGResourceManager::BufferAccess::STATIC, true)
{
	if (VAO == 0) setupMeshVertexArrayObject();

	VBO.setAllObjects(vertices);
	VBO.updateBuffer();

	size_t num_frags = 0;
	std::vector<Fragment> frags;
	for (const MaterialFragments& frag_struct : material_frags) {
		draw_commands.emplace_back(frag_struct.material_index, num_frags * fragment_component_count, frag_struct.frags.size() * fragment_component_count);
		frags.insert(frags.end(), frag_struct.frags.begin(), frag_struct.frags.end());
		num_frags += frag_struct.frags.size();
	}
	EBO.setAllObjects(frags);
	EBO.updateBuffer();

	material_buffer.setAllObjects(materials);
	material_buffer.updateBuffer();

	setupMeshBoundingBox();

	generatePlanes();
	generateBVH();
}

TAGMesh::TAGMesh(const std::vector<Vertex>& vertices, const std::vector<Fragment>& frags, const Material& material) :
	VBO(vertices.size(), convertVertex, TAGResourceManager::BufferAccess::STATIC),
	EBO(frags.size(), convertFragment, TAGResourceManager::BufferAccess::STATIC),
	material_buffer(1, convertMaterial, TAGResourceManager::BufferAccess::STATIC, true)
{
	if (VAO == 0) setupMeshVertexArrayObject();

	VBO.setAllObjects(vertices);
	VBO.updateBuffer();

	EBO.setAllObjects(frags);
	EBO.updateBuffer();
	draw_commands.emplace_back(0, 0, frags.size() * fragment_component_count);

	material_buffer.pushObject(material);
	material_buffer.updateBuffer();

	setupMeshBoundingBox();

	generatePlanes();
	generateBVH();
}

TAGTexLoader::Texture& TAGMesh::Material::getTexture(const std::string& name) {
	return *std::find_if(textures.begin(), textures.end(), [&name](const TAGTexLoader::Texture& tex) { return name == tex.name; });
}

bool TAGMesh::BoundingBox::collisionPoint(const glm::vec3& point) const {
	return glm::clamp(point, min, max) == point;
}

bool TAGMesh::BoundingBox::collisionBBox(const BoundingBox& box) const {
	return max.x >= box.min.x && min.x <= box.max.x &&
		max.y >= box.min.y && min.y <= box.max.y &&
		max.z >= box.min.z && min.z <= box.max.z;
}

bool TAGMesh::BoundingBox::collisionRay(const glm::vec3& start, const glm::vec3& ray, const float& t) const {
	glm::vec3 inv_ray = 1.0f / ray;
	float t_min = -std::numeric_limits<float>::infinity();
	float t_max = -t_min;
	for (size_t i = 0; i < 3; i++) {
		float t1 = (min[i] - start[i]) * inv_ray[i];
		float t2 = (max[i] - start[i]) * inv_ray[i];

		if (t1 > t2) std::swap(t1, t2);

		t_min = std::max(t_min, t1);
		t_max = std::min(t_max, t2);

		if (t_min > t_max) return false; 
	}
	return (t_max >= 0.0f && (t_max <= t || t < 0.0f));
}

bool TAGMesh::BoundingBox::collisionSphere(const glm::vec3& centre, const float& radius) const {
	return TAGUtil::lengthSq(glm::clamp(centre, min, max) - centre) <= radius * radius;
}

bool TAGMesh::BoundingBox::collisionCapsule(const glm::vec3& foot, const glm::vec3& spine, const float& radius) const {
	glm::vec3 closest_ray = foot, closest_box;
	const float s2 = TAGUtil::lengthSq(spine);

	for (int i = 0; i < 4; i++) {
		closest_box = glm::clamp(closest_ray, min, max);
		closest_ray = foot + spine * std::clamp(glm::dot(closest_box - foot, spine) / s2, 0.0f, 1.0f);
	}

	return (glm::abs(TAGUtil::lengthSq(closest_ray - closest_box)) <= radius * radius);
}

bool TAGMesh::Plane::collisionBBox(const BoundingBox& box) const {
	const glm::vec3 half_extents = (box.max - box.min) / 2.0f;
	const std::array<glm::vec3, 3> frag_vertices = { start - box.max + half_extents, start + axis[0] - box.max + half_extents, start + axis[1] - box.max + half_extents };

	// Check aabb of frag against box
	for (size_t i = 0; i < 3; i++) {
		float min = glm::min(glm::min(frag_vertices[0][i], frag_vertices[1][i]), frag_vertices[2][i]);
		float max = glm::max(glm::max(frag_vertices[0][i], frag_vertices[1][i]), frag_vertices[2][i]);

		if (min > half_extents[i] || max < -half_extents[i]) return false;
	}

	// Check box against infinte plane containing frag
	const float d = glm::dot(normal, frag_vertices[0]);
	const float r = glm::dot(half_extents, glm::abs(normal)); // box radius on this axis
	if (d - r > 0 || d + r < 0) return false;

	// Some math I don't understand
	for (size_t i = 0; i < 3; i++) {
		glm::vec3 axis{};
		axis[i] = 1.0f;
		for (size_t j = 0; j < 3; j++) {
			const glm::vec3 edge_normal = glm::cross(frag_vertices[(j == 2 ? 0 : j + 1)] - frag_vertices[j], axis);
			const float p0 = glm::dot(frag_vertices[j + 2 - (j == 0 ? 0 : 3)], edge_normal);
			const float p1 = glm::dot(frag_vertices[j], edge_normal);

			if (glm::max(-glm::max(p0, p1), glm::min(p0, p1)) > glm::dot(half_extents, glm::abs(edge_normal))) return false;
		}
	}

	return true;
}

bool TAGMesh::Plane::collisionPoint(const glm::vec3& point) const {
	const std::array<glm::vec3, 3> cross_prod = {
		glm::cross(point - start, axis[0]),
		glm::cross(point - start - axis[0], axis[1] - axis[0]),
		glm::cross(point - start - axis[1], -axis[1])
	};
	return (glm::dot(cross_prod[0], normal) <= 0.0f && glm::dot(cross_prod[1], normal) <= 0.0f && glm::dot(cross_prod[2], normal) <= 0.0f);
}

bool TAGMesh::Plane::collisionRay(const glm::vec3& start, const glm::vec3& ray, const float& t) const {
	float d = glm::dot(normal, ray);

	if (glm::abs(d) < 0.0001f) return false;

	d = glm::dot(normal, this->start - start) / d;

	if (d < 0.0f || (t < 0.0f && d > t)) return false;

	return collisionPoint(start + ray * d);
}

TAGMesh::DotPlane TAGMesh::PlaneVolume::collisionSphere(const glm::vec3& centre, const float& radius) const {
	GLuint on_edge = 0, on_frag = 0;

	const float dist = glm::dot(frag_plane.normal, centre - frag_plane.start);
	if (glm::abs(dist) > radius) return {};

	for (size_t i = 0; i < 3; i++) {
		const float dist = glm::dot(centre, volume_planes[i].normal) - volume_planes[i].constant;

		if (dist > radius) {
			return {};
		}
		else if (dist > 0.0f) {
			on_edge = i;
		}
		else {
			on_frag++;
		}
	}

	if (on_frag != 3) {
		const glm::vec3 edge_start = frag_plane.start + (on_edge == 1 ? frag_plane.axis[0] : glm::vec3(0.0f));
		const glm::vec3 edge_ray = (on_edge == 0 ? frag_plane.axis[0] : (on_edge == 1 ? frag_plane.axis[1] - frag_plane.axis[0] : frag_plane.axis[1]));

		const glm::vec3 norm = glm::normalize(centre - edge_start - edge_ray * glm::clamp(glm::dot(centre - edge_start, edge_ray) / glm::dot(edge_ray, edge_ray), 0.0f, 1.0f));

		return { norm, glm::dot(norm, edge_start) };
	} 
	return { frag_plane.normal * (dist < 0.0f ? -1.0f : 1.0f), glm::dot(frag_plane.normal * (dist < 0.0f ? -1.0f : 1.0f), frag_plane.start)};
}

TAGMesh::DotPlane TAGMesh::PlaneVolume::collisionCapsule(const glm::vec3& foot, const glm::vec3& spine, const float& radius) const {
	const float spine_normal_dot = glm::dot(spine, frag_plane.normal);
	const float foot_dist = glm::dot(frag_plane.normal, foot - frag_plane.start);

	// All perpendicular cases collide with the infinite plane
	if (glm::abs(spine_normal_dot) < 0.0001f && glm::abs(foot_dist) > radius) return {};

	// Find parameter range along spine where capsule is within edge planes
	float t_lo = 0.0f, t_hi = 1.0f;
	for (size_t i = 0; i < 3; i++) {
		const DotPlane& plane = volume_planes[i];
		const float d0 = glm::dot(foot, plane.normal) - plane.constant;
		const float d1 = glm::dot(foot + spine, plane.normal) - plane.constant;
		const float denom = d1 - d0;

		if (glm::abs(denom) < 0.0001f) {
			if (d0 > 0.0f) { // Entire segment is outside frag
				t_lo = 1.0f;
				t_hi = 0.0f;
				break;
			}
		}
		else {
			const float t = -d0 / denom;
			if (denom > 0.0f) {
				t_hi = glm::min(t_hi, t);
			}
			else {
				t_lo = glm::max(t_lo, t);
			}
		}
	}

	DotPlane best_plane{};
	float best_dist_sq = radius * radius;
	bool found = false;

	if (t_lo <= t_hi) { // Capsule clips into face prism, so check if capsule is close enough to infinite plane
		float h;
		if (glm::abs(spine_normal_dot) < 0.0001f) {
			h = glm::dot(frag_plane.normal, foot - frag_plane.start);
		}
		else {
			const float spine_t = glm::clamp(
				glm::dot(frag_plane.start - foot, frag_plane.normal) / spine_normal_dot, t_lo, t_hi);
			h = glm::dot(frag_plane.normal, foot + spine * spine_t - frag_plane.start);
		}

		const float dist_sq = h * h;
		if (dist_sq < best_dist_sq) {
			best_dist_sq = dist_sq;
			best_plane = { frag_plane.normal * (foot_dist < 0.0f ? -1.0f : 1.0f), glm::dot(frag_plane.normal * (foot_dist < 0.0f ? -1.0f : 1.0f), frag_plane.start)};
			found = true;
		}
	}

	// No face overlap so find closest edge, or there was face overlap but that region does not hit face but outside region could still clip
	for (size_t i = 0; i < 3; i++) {
		const DotPlane& plane = volume_planes[i];
		const glm::vec3 edge_start = frag_plane.start + (i == 1 ? frag_plane.axis[0] : glm::vec3(0.0f));
		const glm::vec3 edge_ray = (i == 0 ? frag_plane.axis[0] : (i == 1 ? frag_plane.axis[1] - frag_plane.axis[0] : frag_plane.axis[1]));

		const float dd = glm::dot(spine, plane.normal);

		glm::vec3 point, point_on_edge;
		if (glm::abs(dd) < 0.0001f) {
			point = foot;
			point_on_edge = edge_start + edge_ray * glm::clamp(
				glm::dot(foot - edge_start, edge_ray) / glm::dot(edge_ray, edge_ray), 0.0f, 1.0f);
		}
		else {
			const glm::vec2 res = glm::inverse(glm::mat2x2(
				glm::dot(edge_ray, spine), glm::dot(spine, spine),
				-glm::dot(edge_ray, edge_ray), -glm::dot(edge_ray, spine)))
				* glm::vec2(glm::dot(edge_start - foot, edge_ray), glm::dot(edge_start - foot, spine));

			point = foot + spine * glm::clamp(res.x, 0.0f, 1.0f);
			point_on_edge = edge_start + edge_ray * glm::clamp(res.y, 0.0f, 1.0f);
		}

		const glm::vec3 sep = point - point_on_edge;
		const float dist_sq = glm::dot(sep, sep);

		if (dist_sq < best_dist_sq) {
			best_dist_sq = dist_sq;
			const glm::vec3 norm = glm::normalize(sep);
			best_plane = { norm, glm::dot(norm, point_on_edge) };
			found = true;
		}
	}

	return (found ? best_plane : DotPlane());
}

GLuint TAGMesh::getVertexBufferID() const { return VBO.getBuffer()->getBufferID(); };
GLuint TAGMesh::getFragmentBufferID() const { return EBO.getBuffer()->getBufferID(); };
GLuint TAGMesh::getMaterialBufferID() const { return material_buffer.getBuffer()->getBufferID(); };
TAGMesh::BoundingBox& TAGMesh::getBoundingBox() { return mesh_bb; };
const TAGMesh::BoundingBox& TAGMesh::getBoundingBox() const { return mesh_bb; };
std::vector<TAGMesh::PlaneVolume>& TAGMesh::getPlanes() { return planes; };
const std::vector<TAGMesh::PlaneVolume>& TAGMesh::getPlanes() const { return planes; };
std::vector<TAGMesh::BVHNode>& TAGMesh::getBVH() { return bvh_octree; };
const std::vector<TAGMesh::BVHNode>& TAGMesh::getBVH() const { return bvh_octree; };
TAGMesh::VertexBuffer& TAGMesh::getVertexBuffer() { return VBO; };
const TAGMesh::VertexBuffer& TAGMesh::getVertexBuffer() const { return VBO; };
TAGMesh::FragmentBuffer& TAGMesh::getFragmentBuffer() { return EBO; };
const TAGMesh::FragmentBuffer& TAGMesh::getFragmentBuffer() const { return EBO; };
TAGMesh::MaterialBuffer& TAGMesh::getMaterialBuffer() { return material_buffer; };
const TAGMesh::MaterialBuffer& TAGMesh::getMaterialBuffer() const { return material_buffer; };

void TAGMesh::draw(const TAGShaderManager::Shader& shader, const TAGShaderManager::ShaderOptions& options, const GLuint& number) {
	// Update buffers if client side objects have changed
	if (VBO.isObjectsChanged()) VBO.updateBuffer();
	if (EBO.isObjectsChanged()) EBO.updateBuffer();
	if (material_buffer.isObjectsChanged()) material_buffer.updateBuffer();

	// Activate global VAO
	glBindVertexArray(VAO);

	// Bind mesh VBO to global VAO
	glBindVertexBuffer(default_vao_mesh_binding_point, getVertexBufferID(), 0, sizeof(Vertex));

	// Bind mesh EBO to global VAO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, getFragmentBufferID());

	for (const PerMaterialDrawCommand& draw_command : draw_commands) {
		// Bind current material to global VAO
		glBindVertexBuffer(defualt_vao_material_binding_point, getMaterialBufferID(), draw_command.material_index * sizeof(ShaderMaterial) + sizeof(GLuint), sizeof(ShaderMaterial));
		glVertexAttribDivisor(defualt_vao_material_binding_point, number);

		std::vector<GLuint> diffuse, specular;
		const Material& material = material_buffer.getObject(draw_command.material_index);
		for (GLuint i = 0; i < material.textures.size(); i++) {
			glActiveTexture(GL_TEXTURE0 + i);
			switch (material.textures[i].type) {
			case TAGTexType::DIFFUSE_MAP:
				if (diffuse.size() < 16) {
					diffuse.push_back(i);
				}
				break;
			case TAGTexType::SPEC_MAP:
				if (specular.size() < 16) {
					specular.push_back(i);
				}
			}
			glBindTexture(GL_TEXTURE_2D, material.textures[i].id);
		}
		glActiveTexture(GL_TEXTURE0);

		if (!diffuse.empty()) shader.set<ShaderUniformType::SINGLE_2D>(options.diffuse_tex_array, diffuse[0], (GLuint)diffuse.size());
		if (!specular.empty()) shader.set<ShaderUniformType::SINGLE_2D>(options.specular_tex_array, specular[0], (GLuint)specular.size());
		shader.set<glm::vec3>(options.camera_pos, TAGBaseState::camera_position);

		glDrawElementsInstanced(GL_TRIANGLES, draw_command.element_count, GL_UNSIGNED_INT, (void*)(draw_command.element_start * sizeof(GLuint)), number);
		VBO.setFence();
		EBO.setFence();
		material_buffer.setFence();
	}
	// Reset VAO state
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glVertexAttribDivisor(defualt_vao_material_binding_point, 1);
	glBindVertexBuffer(defualt_vao_material_binding_point, 0, 0, 0);
	glBindVertexBuffer(default_vao_mesh_binding_point, 0, 0, 0);
	glBindVertexArray(0);
}

void TAGMesh::setupMeshBoundingBox() {
	const std::vector<Vertex>& vertices = VBO.getAllObjects();
	mesh_bb = { vertices[0].position, vertices[0].position };
	for (const TAGMesh::Vertex& vertex : vertices) {
		mesh_bb.max = glm::max(mesh_bb.max, vertex.position);
		mesh_bb.min = glm::min(mesh_bb.min, vertex.position);
	}
}

void TAGMesh::setupMeshVertexArrayObject() {
	VAO = TAGResourceManager::createBuffer<OpenGLObjectType::VertexArrayObject>();

	glBindVertexArray(VAO);

	// Mesh vertex attributes
	GLuint attrib = base_attrib;
	glEnableVertexAttribArray(attrib);
	glVertexAttribFormat(attrib, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexAttribBinding(attrib++, default_vao_mesh_binding_point);
	glEnableVertexAttribArray(attrib);
	glVertexAttribFormat(attrib, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
	glVertexAttribBinding(attrib++, default_vao_mesh_binding_point);
	glEnableVertexAttribArray(attrib);
	glVertexAttribFormat(attrib, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, tex_coord));
	glVertexAttribBinding(attrib++, default_vao_mesh_binding_point);

	// Instance attributes
	for (GLuint i = 0; i < 2; i++) {
		glEnableVertexAttribArray(attrib);
		glVertexAttribFormat(attrib, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4) * i);
		glVertexAttribBinding(attrib++, default_vao_instance_binding_point);
	}
	glVertexBindingDivisor(default_vao_instance_binding_point, 1);

	// Material attributes
	glEnableVertexAttribArray(attrib);
	glVertexAttribFormat(attrib, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexAttribBinding(attrib++, defualt_vao_material_binding_point);
	glEnableVertexAttribArray(attrib);
	glVertexAttribFormat(attrib, 3, GL_FLOAT, GL_FALSE, offsetof(ShaderMaterial, colour));
	glVertexAttribBinding(attrib++, defualt_vao_material_binding_point);
	glEnableVertexAttribArray(attrib);
	glVertexAttribIFormat(attrib, 2, GL_UNSIGNED_INT, offsetof(ShaderMaterial, num_diffuse));
	glVertexAttribBinding(attrib, defualt_vao_material_binding_point);
	glVertexBindingDivisor(defualt_vao_material_binding_point, 1);

	glBindVertexArray(0);
}

std::array<GLfloat, TAGMesh::vertex_component_count> TAGMesh::convertVertex(const Vertex& vertex) {
	return { 
		 vertex.position.x, vertex.position.y, vertex.position.z,
		 vertex.normal.x, vertex.normal.y, vertex.normal.z,
		 vertex.tex_coord.x, vertex.tex_coord.y 
	};
}

std::array<GLuint, TAGMesh::fragment_component_count> TAGMesh::convertFragment(const Fragment& frag) {
	return { frag[0], frag[1], frag[2] };
}

std::array<TAGMesh::ShaderMaterial, 1> TAGMesh::convertMaterial(const Material& material) {
	const TAGMesh::ShaderMaterial shader_material = {
		material.spec_fac, material.spec_exp, material.opacity,
		material.colour,
		static_cast<GLuint>(std::count_if(material.textures.begin(), material.textures.end(), [](const TAGTexLoader::Texture& tex) { return tex.type == TAGTexType::DIFFUSE_MAP; })),
		static_cast<GLuint>(std::count_if(material.textures.begin(), material.textures.end(), [](const TAGTexLoader::Texture& tex) { return tex.type == TAGTexType::SPEC_MAP; }))
	};
	return { shader_material };
}

TAGMesh::BoundingBox TAGMesh::generateBoundingBox(const glm::vec3* first, const unsigned int& size) {
	glm::vec3 max, min;
	max = min = *first;
	for (unsigned int i = 1; i < size; i++) {
		max = glm::max(max, *(first + i));
		min = glm::min(min, *(first + i));
	}
	return { max, min };
}

void TAGMesh::generatePlanes() {
	planes.clear();
	planes.reserve(EBO.getAllObjects().size());

	const std::vector<Vertex>& vertices = VBO.getAllObjects();

	for (const Fragment& frag : EBO.getAllObjects()) {
		std::array<glm::vec3, 3> frag_vertices{};
		for (size_t i = 0; i < 3; i++) {
			frag_vertices[i] = vertices[frag[i]].position;
		}
		const std::array<glm::vec3, 2> frag_axis = { frag_vertices[1] - frag_vertices[0], frag_vertices[2] - frag_vertices[0] };
		const glm::vec3 normal = glm::normalize(glm::cross(frag_axis[0], frag_axis[1]));

		const glm::mat3 rotate_mat = glm::mat3(glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), normal)); // Rotate frag sides 90 deg to get normals

		const std::array<glm::vec3, 3> volume_normals = {
			glm::normalize(rotate_mat * frag_axis[0]),
			glm::normalize(rotate_mat * (frag_axis[1] - frag_axis[0])),
			glm::normalize(rotate_mat * -frag_axis[1])
		}; // Outward facing normals of each side

		std::vector<DotPlane> volume_planes;
		volume_planes.reserve(3);
		for (size_t i = 0; i < 3; i++) {
			volume_planes.emplace_back(volume_normals[i], glm::dot(volume_normals[i], frag_vertices[i]));
		}

		planes.push_back(
			{
				.frag_plane = {
					.normal = normal,
					.start = frag_vertices[0],
					.axis = frag_axis
				},
				.volume_planes = *reinterpret_cast<std::array<DotPlane, 3>*>(volume_planes.data())
			}
		);
	}
}

void TAGMesh::generateBVH() {
	bvh_octree.clear();

	// Info for BVH nodes to be processed during construction
	struct BVHQueue {
		size_t node_index, layer_index, depth;
	};

	// Function that splits bounding box into 8 equally sized bounding boxes and pushes them to bvh tree
	auto octGen = [this](BoundingBox box) {
		const glm::vec3 c = (box.max + box.min) / 2.0f;
		for (size_t i = 0; i < 8; i++) {
			glm::vec3 min, max;

			min.x = (i & 1) ? c.x : box.min.x;
			max.x = (i & 1) ? box.max.x : c.x;

			min.y = (i & 2) ? c.y : box.min.y;
			max.y = (i & 2) ? box.max.y : c.y;

			min.z = (i & 4) ? c.z : box.min.z;
			max.z = (i & 4) ? box.max.z : c.z;

			this->bvh_octree.emplace_back(true, BoundingBox(max, min));
		}
		};

	// Creates a range of values
	auto range = [](const size_t& min, const size_t& max) {
		std::vector<size_t> nums(max - min);
		for (size_t i = min; i < max; i++) {
			nums[i] = i;
		}
		return nums;
		};

	// The available planes at particular depths in the BVH
	std::vector<std::vector<size_t>> layers;

	// Queue for holding BVH boxes yet to be processed
	std::deque<BVHQueue> bvh_queue;

	// Split mesh aabb to avoid checking all plane aabbs since we know planes are already in mesh
	const std::vector<size_t> all_plane_indices = range(0, planes.size());
	if (planes.size() <= bvh_box_max_size) {
		bvh_octree.emplace_back(true, mesh_bb, all_plane_indices);
		return;
	}
	else {
		std::vector<size_t> node_children(8);
		for (size_t i = 1; i < 9; i++) {
			bvh_queue.push_back({ i, 0, 0 });
			node_children[i - 1] = i;
		}
		bvh_octree.emplace_back(false, mesh_bb, node_children);
		layers.push_back(all_plane_indices);
		octGen(mesh_bb);
	}

	while (!bvh_queue.empty()) {
		// Next BVH box is removed from front of queue
		BVHQueue current_info = bvh_queue.front();
		BVHNode current_box = bvh_octree[current_info.node_index];
		std::vector<size_t>& layer_indices = layers[current_info.layer_index];

		// Find which planes within parent bounding box collide with current box
		for (const size_t& plane_index : layer_indices) {
			if (planes[plane_index].frag_plane.collisionBBox(current_box.bounds)) {
				current_box.indices.push_back(plane_index);
			}
		}

		// split if too big and depth isnt too deep
		if (current_box.indices.size() > bvh_box_max_size && current_info.depth < bvh_max_depth) {
			layers.push_back(current_box.indices);
			current_box.indices.clear();
			for (size_t i = bvh_octree.size(); i < bvh_octree.size() + 8; i++) {
				bvh_queue.push_back({ i, layers.size() - 1, current_info.depth + 1 });
				current_box.indices.push_back(i);
			}
			current_box.is_leaf = false;
			octGen(current_box.bounds);
		}

		bvh_queue.erase(bvh_queue.begin());
		bvh_octree[current_info.node_index] = current_box;
	}

	// Clean tree a bit
	for (size_t i = 0; i < bvh_octree.size(); i++) {
		BVHNode& node = bvh_octree[i];
		if (!node.is_leaf) {
			// Don't point to empty children
			for (size_t j = 0; j < node.indices.size(); j++) {
				if (bvh_octree[node.indices[j]].indices.empty()) {
					node.indices.erase(node.indices.begin() + j--);
				}
			}

			// Swap if only child
			if (node.indices.size() == 1) {
				const size_t only_child = node.indices[0];
				node.indices.clear();
				std::swap(node, bvh_octree[only_child]);
				i--;
			}
		}
	}

	bvh_octree.shrink_to_fit();
}
