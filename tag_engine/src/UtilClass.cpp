#include <UtilClass.hpp>

glm::vec3 TAGUtil::parallelComp(const glm::vec3& ref, const glm::vec3& vec) {
	float dot = glm::dot(ref, vec);
	if (dot < 0.0f) dot *= bump_factor;
	return ref * (dot / lengthSq(ref));
}

glm::vec3 TAGUtil::perpendicularComp(const glm::vec3& ref, const glm::vec3& vec) {
	return vec - parallelComp(ref, vec);
}

glm::vec3 TAGUtil::parallelCompNorm(const glm::vec3& ref, const glm::vec3& vec) {
	float dot = glm::dot(ref, vec);
	if (dot < 0.0f) dot *= bump_factor;
	return ref * dot;
}

glm::vec3 TAGUtil::perpendicularCompNorm(const glm::vec3& ref, const glm::vec3& vec) {
	return vec - parallelCompNorm(ref, vec);
}

glm::vec4 TAGUtil::pointDirection(const glm::vec3& direction, const glm::vec3& abs_direction) {
	const glm::vec3 cross = glm::cross(abs_direction, direction);
	const float cross_length = glm::length(cross);
	return { (cross_length > 0.0f ? cross / cross_length : glm::vec3(0.0f, 1.0f, 0.0f)),
		std::atan2(cross_length, glm::dot(abs_direction, direction)) };
}

float TAGUtil::lengthSq(const glm::vec3& vec) {
	return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}
