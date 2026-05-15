#pragma once
#include <random>
#include <concepts>
#include <glm/glm.hpp>

/**
* Tri state constant for manipulating certain states easier
*/
enum class TAGEnum {
	TRUE,
	FALSE,
	TOGGLE
};

/**
* Floating point number only concept
*/
template <typename T> concept Floating = std::is_floating_point_v<T>;

/**
* Integral number only concept
*/
template <typename T> concept Integral = std::is_integral_v<T>;

/**
* Variadic helper template for defining other templates 
*/
template <typename T, typename... Ts> concept isAnyOf = (std::same_as<T, Ts> || ...);

/**
* Struct for checking if type is in std::variant
*/
template <typename T, typename Variant>
struct isVariantMember;

template <typename T, typename... Ts>
struct isVariantMember<T, std::variant<Ts...>>
	: std::bool_constant<(std::same_as<T, Ts> || ...)> {
};

/**
* Provides useful functions
*/
class TAGUtil {
public:
	/**
	* Perpendicular component of vec to ref.
	* 
	* @param ref Reference vector to find a perpendicular vector to
	* @param vec Vector to find component of
	*/
	static glm::vec3 perpendicularComp(const glm::vec3& ref, const glm::vec3& vec);
	/**
	* Perpendicular component of vec to ref.
	* ref is assumed to be normalized which makes the operation faster.
	*
	* @param ref Reference vector to find a perpendicular vector to
	* @param vec Vector to find component of
	*/
	static glm::vec3 perpendicularCompNorm(const glm::vec3& ref, const glm::vec3& vec);
	/**
	* Parallel component of vec to ref.
	*
	* @param ref Reference vector to find a parallel vector to
	* @param vec Vector to find component of
	*/
	static glm::vec3 parallelComp(const glm::vec3& ref, const glm::vec3& vec);
	/**
	* Parallel component of vec to ref.
	* ref is assumed to be normalized which makes the operation faster.
	*
	* @param ref Reference vector to find a parallel vector to
	* @param vec Vector to find component of
	*/
	static glm::vec3 parallelCompNorm(const glm::vec3& ref, const glm::vec3& vec);
	/**
	* Find the squared length of vec.
	* 
	* @param vec 3D vector
	*/
	static float lengthSq(const glm::vec3& vec);
	/**
	* Random floating point value between 0 and 1 of type T.
	*/
	template<Floating T> static T randomFloat();
	/**
	* Random integer value between min and max of type T.
	* 
	* @param min Minimum value (inclusive)
	* @param min Maximum value (inclusive)
	*/
	template<Integral T> static T randomRangeInt(const T& min, const T& max);
	/**
	* Random floating point value between min and max of type T.
	*
	* @param min Minimum value (inclusive)
	* @param min Maximum value (inclusive)
	*/
	template<Floating T> static T randomRangeFloat(const T& min, const T& max);
private:
	static inline std::mt19937 gen{ std::random_device{}() };
};

#include "../../src/UtilClass.inl"