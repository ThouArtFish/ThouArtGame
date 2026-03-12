#pragma once

#include <ResourceManagerClass.hpp>

template<BufferType T> void TAGResourceManager::deleteBuffer(const unsigned int& ID) {
	const auto it = std::find_if(buffers.begin(), buffers.end(), [&ID](const BufferVariant& buf) 
		{ 
			if (std::holds_alternative<T>(buf)) {
				return (std::get<T>(buf).ID == ID);
			}
			else {
				return false;
			}
		}
	);
	if (it != buffers.end()) {
		buffers.erase(it);
	}
}

template<BufferType T> unsigned int TAGResourceManager::createBuffer() {
	buffers.emplace_back(std::in_place_type<T>);
	return std::get<T>(buffers.back()).ID;
}