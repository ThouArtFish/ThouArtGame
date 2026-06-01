#pragma once

#include <HUDManagerClass.hpp>

template<typename T, unsigned int POSITION> requires isAnyOf<T, glm::vec2, std::string, TAGHUDManager::DimensionFormat> static unsigned int TAGHUDManager::quadMemberOffset() {
	if constexpr (std::same_as<T, glm::vec2>) {
		if constexpr (POSITION == 0) {
			return offsetof(Quad, position);
		}
		else if constexpr (POSITION == 1) {
			return offsetof(Quad, dimensions);
		}
		else if constexpr (position == 2) {
			return offsetof(Quad, texel_top_left);
		}
		else {
			return offsetof(Quad, texel_bottom_right);
		}
	}
	else if constexpr (std::same_as<T, std::string>) {
		return offsetof(Quad, image_name);
	}
	else {
		if constexpr (POSITION == 0) {
			return offsetof(Quad, position_format);
		}
		else if constexpr (POSITION == 1) {
			return offsetof(Quad, dimension_format);
		}
		else {
			return offsetof(Quad, texel_format);
		}
	}
}