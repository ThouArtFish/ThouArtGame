#pragma once

#include <HUDManagerClass.hpp>

template<QuadMemberName::Concept T> void TAGHUDManager::setQuadMember(const T::TYPE& value, const GLuint& index) {
	quads.setObjectMember<typename T::TYPE>(value, T::OFFSET, index);
}