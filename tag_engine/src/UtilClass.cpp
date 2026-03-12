#include <UtilClass.hpp>

glm::vec3 TAGUtil::parallelComp(const glm::vec3& ref, const glm::vec3& vec) {
	return ref * (glm::dot(ref, vec) / glm::dot(ref, ref));
}

glm::vec3 TAGUtil::perpendicularComp(const glm::vec3& ref, const glm::vec3& vec) {
	return vec - parallelComp(ref, vec);
}

glm::vec3 TAGUtil::parallelCompNorm(const glm::vec3& ref, const glm::vec3& vec) {
	return ref * glm::dot(ref, vec);
}

glm::vec3 TAGUtil::perpendicularCompNorm(const glm::vec3& ref, const glm::vec3& vec) {
	return vec - parallelCompNorm(ref, vec);
}

float TAGUtil::lengthSq(const glm::vec3& vec) {
	return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}
