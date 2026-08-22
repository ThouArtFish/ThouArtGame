#include <MeshClass.hpp>

TAGMesh::TAGMesh(const std::vector<Vertex>& vertices, const std::vector<Fragment>& frags, const std::vector<Material>& materials) {
	this->vertices = vertices;
	this->frags = frags;
	this->materials = materials;

	setupMesh();
}

TAGMesh::TAGMesh() {}

TAGMesh::~TAGMesh() {
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(VBO);
	for (const MaterialElementBuffer& material_ebo : material_ebos) {
		TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(material_ebo.EBO);
	}
	TAGResourceManager::deleteBuffer<OpenGLObjectType::VertexArrayObject>(VAO);
}

TAGTexLoader::Texture& TAGMesh::Material::getTexture(const std::string& name) {
	return *std::find_if(textures.begin(), textures.end(), [&name](const TAGTexLoader::Texture& tex) { return name == tex.name; });
}

TAGMesh::MaterialElementBuffer::~MaterialElementBuffer() {
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(EBO);
}

void TAGMesh::generatePlanes() {
	planes.clear();
	planes.reserve(frags.size());

	for (const TAGMesh::Fragment& frag_struct : frags) {
		const std::array<GLuint, 3> frag = frag_struct.vertex_indices;
		const std::array<glm::vec3, 3> frag_vertices = { vertices[frag[0]].position, vertices[frag[1]].position, vertices[frag[2]].position };
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
		GLuint node_index, layer_index, depth;
	};

	// Get mesh bounding box
	mesh_bb = { vertices[0].position, vertices[0].position };
	for (const TAGMesh::Vertex& vertex : vertices) {
		mesh_bb.max = glm::max(mesh_bb.max, vertex.position);
		mesh_bb.min = glm::min(mesh_bb.min, vertex.position);
	}

	// Plane bounding boxes
	std::vector<BoundingBox> plane_bb;
	plane_bb.reserve(frags.size());
	for (const Fragment& frag_struct : frags) {
		const std::array<unsigned int, 3> frag = frag_struct.vertex_indices;
		const std::array<glm::vec3, 3> frag_vertices = {
			vertices[frag[0]].position,
			vertices[frag[1]].position,
			vertices[frag[2]].position
		};
		plane_bb.push_back(generateBoundingBox(frag_vertices.data(), 3));
	}

	// Function that splits bounding box into 8 equally sized bounding boxes
	auto octGen = [this](const BoundingBox& box) {
		const glm::vec3 c = (box.max + box.min) / 2.0f;
		for (unsigned int i = 0; i < 8; i++) {
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
	auto range = [](const unsigned int& min, const unsigned int& max) {
		std::vector<unsigned int> nums;
		nums.reserve(max - min);
		for (unsigned int i = min; i < max; i++) {
			nums.push_back(i);;
		}
		return nums;
	};

	// The available planes at particular depths in the BVH
	std::vector<std::vector<unsigned int>> layers;

	// Queue for holding BVH boxes yet to be processed
	std::vector<BVHQueue> bvh_queue;

	// Split mesh aabb to avoid checking all plane aabbs since we know planes are already in mesh
	const std::vector<unsigned int> all_plane_indices = range(0, (unsigned int)planes.size());
	if (planes.size() <= bvh_box_max_size) {
		bvh_octree.emplace_back(true, mesh_bb, all_plane_indices);
		return;
	}
	else {
		std::vector<unsigned int> node_children;
		node_children.reserve(8);
		for (unsigned int i = 1; i < 9; i++) {
			bvh_queue.push_back({ i, 0, 0 });
			node_children.push_back(i);
		}
		bvh_octree.emplace_back(false, mesh_bb, node_children);
		layers.push_back(all_plane_indices);
		octGen(mesh_bb);
	}

	while (!bvh_queue.empty()) {
		// Next BVH box is removed from front of queue
		BVHQueue& current_info = bvh_queue.front();
		BVHNode& current_box = bvh_octree.at(current_info.node_index);
		std::vector<unsigned int>& layer_indices = layers.at(current_info.layer_index);

		// Find which planes within parent bounding box collide with current box
		for (const unsigned int& plane_index : layer_indices) {
			if (BBoxWithBBox(current_box.bounds, plane_bb[plane_index])) {
				current_box.indices.push_back(plane_index);
			}
		}

		// split if too big and depth isnt too deep
		if (current_box.indices.size() > bvh_box_max_size && current_info.depth < 3) {
			layers.push_back(current_box.indices);
			current_box.indices.clear();
			for (unsigned int i = (unsigned int)bvh_octree.size(); i < (unsigned int)bvh_octree.size() + 8; i++) {
				bvh_queue.push_back({ i, (unsigned int)layers.size() - 1, current_info.depth + 1});
				current_box.indices.push_back(i);
			}
			current_box.is_leaf = false;
			octGen(current_box.bounds);
		}

		bvh_queue.erase(bvh_queue.begin());
	}

	// Clean the tree a bit
	for (int i = 0; i < bvh_octree.size(); i++) {
		BVHNode& node = bvh_octree[i];
		if (!node.is_leaf) {
			for (int j = 0; j < node.indices.size(); j++) {
				if (bvh_octree[node.indices[j]].indices.empty()) {
					node.indices.erase(node.indices.begin() + j);
					j--;
				}
			}

			if (node.indices.size() == 1) {
				node.is_leaf = true;
				std::swap(node, bvh_octree[node.indices[0]]);
				i--;
			}
		}
		node.indices.shrink_to_fit();
	}
}

void TAGMesh::setupMesh() {
	TAGResourceManager::deleteBuffer<OpenGLObjectType::GenericBuffer>(VBO);
	TAGResourceManager::deleteBuffer<OpenGLObjectType::VertexArrayObject>(VAO);
	material_ebos.clear();

	std::unordered_map<unsigned int, std::vector<std::array<unsigned int, 3>>> material_frags;
	for (const Fragment& frag_struct : frags) {
		material_frags[frag_struct.material_index].push_back(frag_struct.vertex_indices);
	}
	for (const auto& pair : material_frags) {
		material_ebos.emplace_back(TAGResourceManager::createBuffer<OpenGLObjectType::GenericBuffer>(), pair.first);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, material_ebos.back().EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, pair.second.size() * sizeof(std::array<unsigned int, 3>), pair.second.data(), GL_STATIC_DRAW);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	std::sort(material_ebos.begin(), material_ebos.end(), 
		[this](const MaterialElementBuffer& a, const MaterialElementBuffer& b) {
			return this->materials[a.material_index].opacity > this->materials[b.material_index].opacity;
		}
	);

	VBO = TAGResourceManager::createBuffer<OpenGLObjectType::GenericBuffer>();
	VAO = TAGResourceManager::createBuffer<OpenGLObjectType::VertexArrayObject>();

	glBindVertexArray(VAO);

	glBindVertexBuffer(0, VBO, 0, sizeof(Vertex));
	glNamedBufferData(VBO, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(base_attrib);
	glVertexAttribFormat(base_attrib, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexAttribBinding(base_attrib, 0);
	glEnableVertexAttribArray(base_attrib + 1);
	glVertexAttribFormat(base_attrib + 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
	glVertexAttribBinding(base_attrib + 1, 0);
	glEnableVertexAttribArray(base_attrib + 2);
	glVertexAttribFormat(base_attrib + 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, tex_coord));
	glVertexAttribBinding(base_attrib + 2, 0);

	for (unsigned int i = 0; i < 2; i++) {
		const unsigned int base = base_attrib + i + 3;
		glEnableVertexAttribArray(base);
		glVertexAttribFormat(base, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4) * i);
		glVertexAttribBinding(base, 1);
	}
	glVertexBindingDivisor(1, 1);

	glBindVertexArray(0);

	this->vertices_updated = false;

	generatePlanes();
	generateBVH();
}

void TAGMesh::draw(const TAGShaderManager::Shader& shader, const TAGShaderManager::ShaderOptions& options, const unsigned int& number) {
	if (vertices_updated || frags_updated) {
		if (vertices_updated) {
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			GLint vertices_size;
			glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &vertices_size);
			GLint buffer_size = (GLint)vertices.size() * sizeof(Vertex);
			if (buffer_size > vertices_size) {
				glBufferData(GL_ARRAY_BUFFER, buffer_size, vertices.data(), GL_DYNAMIC_DRAW);
			}
			else {
				glBufferSubData(GL_ARRAY_BUFFER, 0, buffer_size, vertices.data());
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			vertices_updated = false;
		}
		if (frags_updated) {
			material_ebos.clear();
			std::unordered_map<unsigned int, std::vector<std::array<unsigned int, 3>>> material_frags;
			for (const Fragment& frag_struct : frags) {
				material_frags[frag_struct.material_index].push_back(frag_struct.vertex_indices);
			}
			for (const auto& pair : material_frags) {
				material_ebos.emplace_back(TAGResourceManager::createBuffer<OpenGLObjectType::GenericBuffer>(), pair.first);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, material_ebos.back().EBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, pair.second.size() * sizeof(std::array<unsigned int, 3>), pair.second.data(), GL_STATIC_DRAW);
			}
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			std::sort(material_ebos.begin(), material_ebos.end(),
				[this](const MaterialElementBuffer& a, const MaterialElementBuffer& b) {
					return this->materials[a.material_index].opacity > this->materials[b.material_index].opacity;
				}
			);
			frags_updated = false;
		}
		generatePlanes();
		generateBVH();
	}

	glBindVertexArray(VAO);
	for (const MaterialElementBuffer& material_ebo : material_ebos) {
		std::vector<unsigned int> diffuse, specular;
		const Material& material = materials[material_ebo.material_index];
		for (unsigned int i = 0; i < material.textures.size(); i++)
		{
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

		if (diffuse.size() == 0) {
			shader.set<glm::vec3>(options.colour_vec, material.colour);
		}
		else {
			shader.set<ShaderUniformType::SINGLE_2D>(options.diffuse_tex_array, diffuse[0], (GLuint) diffuse.size());
		}
		shader.set<int>(options.diffuse_tex_num, (GLuint) diffuse.size());

		if (material.spec_fac > 0.0f && specular.size() > 0) {
			shader.set<ShaderUniformType::SINGLE_2D>(options.specular_tex_array, specular[0], (GLuint) specular.size());
		}
		shader.set<float>(options.specular_factor, material.spec_fac);
		shader.set<float>(options.specular_exp, material.spec_exp);
		shader.set<int>(options.specular_tex_num, (GLuint) specular.size());

		shader.set<float>(options.opacity_value, material.opacity);
		shader.set<glm::vec3>(options.camera_pos, TAGBaseState::camera_position);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, material_ebo.EBO);
		if (number > 1) {
			glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)(frags.size() * 3), GL_UNSIGNED_INT, nullptr, number);
		}
		else {
			glDrawElements(GL_TRIANGLES, (GLsizei)(frags.size() * 3), GL_UNSIGNED_INT, nullptr);
		}
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

std::vector<TAGMesh::Vertex>& TAGMesh::changeVertices() {
	vertices_updated = true;
	return vertices;
}

const std::vector<TAGMesh::Vertex>& TAGMesh::getVertices() const {
	return vertices;
}

std::vector<TAGMesh::Fragment>& TAGMesh::changeFragments() {
	frags_updated = true;
	return frags;
}

const std::vector<TAGMesh::Fragment>& TAGMesh::getFragments() const {
	return frags;
}

TAGMesh::Material& TAGMesh::getMaterial(const unsigned int& index) {
	return materials.at(index);
}

TAGMesh::Material& TAGMesh::getMaterial(const std::string& name) {
	return *std::find_if(materials.begin(), materials.end(), [&name](const Material& mat) { return mat.name == name; });
}

const unsigned int& TAGMesh::getVAO() const {
	return VAO;
};

const std::vector<TAGMesh::MaterialElementBuffer>& TAGMesh::getEBOs() const {
	return material_ebos;
};

const unsigned int& TAGMesh::getVBO() const {
	return VBO;
};

bool TAGMesh::FragWithPoint(const glm::vec3& point, const Plane& plane) {
	const std::array<glm::vec3, 3> cross_prod = {
		glm::cross(point - plane.start, plane.axis[0]),
		glm::cross(point - plane.start - plane.axis[0], plane.axis[1] - plane.axis[0]),
		glm::cross(point - plane.start - plane.axis[1], -plane.axis[1])
	};
	return (glm::dot(cross_prod[0], plane.normal) <= 0.0f && glm::dot(cross_prod[1], plane.normal) <= 0.0f && glm::dot(cross_prod[2], plane.normal) <= 0.0f);
}

bool TAGMesh::FragWithSphere(const glm::vec3& centre, const float& radius, const PlaneVolume& plane) {
	for (size_t i = 0; i < 4; i++) {
		const double signed_dist = (
			i == 0 ? glm::abs(glm::dot(centre - plane.frag_plane.start, plane.frag_plane.normal))
			: glm::dot(centre, plane.volume_planes[i - 1].normal) - plane.volume_planes[i - 1].constant
		);

		if (signed_dist > radius) {
			return false;
		}
	}
	return true;
}

bool TAGMesh::FragWithCapsule(const glm::vec3& foot, const glm::vec3& spine, const float& radius, const PlaneVolume& plane) {
	double d = glm::dot(spine, plane.frag_plane.normal);
	if (glm::abs(d) < 0.0001) {
		if (glm::abs(glm::dot(plane.frag_plane.normal, foot - plane.frag_plane.start)) > radius) {
			return false;
		}
		for (const DotPlane& volume_plane : plane.volume_planes) {
			d = glm::dot(spine, volume_plane.normal);
			if (glm::abs(d) < 0.0001) {
				if (glm::dot(foot, volume_plane.normal) > radius + volume_plane.constant) {
					return false;
				}
			}
			else {
				d = glm::min(1.0, glm::max(0.0, (volume_plane.constant - glm::dot(foot, volume_plane.normal)) / d));
				if (glm::dot(foot + spine * (float)d, volume_plane.normal) > radius + volume_plane.constant) {
					return false;
				}
			}
		}
		return true;
	}
	return FragWithSphere(foot + spine * (float)glm::min(1.0, glm::max(0.0, glm::dot(plane.frag_plane.normal, plane.frag_plane.start - foot) / d)), radius, plane);
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

bool TAGMesh::BBoxWithBBox(const BoundingBox& box_a, const BoundingBox& box_b) {
	return box_a.max.x >= box_b.min.x && box_a.min.x <= box_b.max.x &&
		box_a.max.y >= box_b.min.y && box_a.min.y <= box_b.max.y &&
		box_a.max.z >= box_b.min.z && box_a.min.z <= box_b.max.z;
}

bool TAGMesh::BBoxWithRay(const BoundingBox& box, const glm::vec3& start, const glm::vec3& ray, const float& factor) {
	double t_exit = std::numeric_limits<double>::infinity();
	double t_enter = -t_exit;
	for (GLuint i = 0; i < 3; i++) {
		if (glm::abs(ray[i]) > 0.0001f) {
			t_exit = glm::min((double)((ray[i] > 0.0f ? box.max[i] : box.min[i]) - start[i]) / ray[i], t_exit);
			t_enter = glm::max((double)((ray[i] > 0.0f ? box.min[i] : box.max[i]) - start[i]) / ray[i], t_enter);
		}
		else if (start[i] < box.min[i] || start[i] > box.max[i]) {
			return false;
		}
	}
	return (t_enter <= t_exit && t_exit >= 0.0 && (t_exit <= factor || factor < 0.0f));
}

bool TAGMesh::BBoxWithCapsule(const BoundingBox& box, const glm::vec3& foot, const glm::vec3& spine, const float& radius) {
	double t_enter = -std::numeric_limits<double>::infinity();
	double t_exit = std::numeric_limits<double>::infinity();
	for (GLuint i = 0; i < 3; i++) {
		if (glm::abs(spine[i]) > 0.0001f) {
			t_enter = glm::max((double)((spine[i] > 0.0f ? box.min[i] : box.max[i]) - radius - foot[i]) / spine[i], t_enter);
			t_exit = glm::min((double)((spine[i] > 0.0f ? box.max[i] : box.min[i]) + radius - foot[i]) / spine[i], t_exit);
		}
		else if (foot[i] < box.min[i] - radius || foot[i] > box.max[i] + radius) {
			return false;
		}
	}
	return (t_enter <= t_exit && t_exit >= 0.0 && t_enter <= 1.0);
}

bool TAGMesh::BBoxWithSphere(const BoundingBox& box, const glm::vec3& centre, const float& radius) {
	return centre.x >= box.min.x - radius && centre.x <= box.max.x + radius &&
		centre.y >= box.min.y - radius && centre.y <= box.max.y + radius &&
		centre.z >= box.min.z - radius && centre.z <= box.max.z + radius;
}
