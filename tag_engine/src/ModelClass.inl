#pragma once 

#include <ModelClass.hpp>

template<ObjectMemberName::Concept T> void TAGModel::setInstanceMember(const T::TYPE& value, const GLuint& index, const std::string& mesh_name) {
	auto& obj_buffer = instance_buffers.at(mesh_name);

	obj_buffer.setObjectMember<typename T::TYPE>(value, T::OFFSET, index);
}