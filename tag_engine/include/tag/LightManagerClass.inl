#pragma once

#include <LightManagerClass.hpp>

template<LightType T> TAGLightManager<T>::TAGLightManager(const std::vector<T>& lights, const TAGResourceManager::BufferAccess& access, const size_t size) {
	this->lights = LightBuffer(glm::max(size, lights.size()), shaderLightConverter, access, true);
	this->lights.setAllObjects(lights);
	this->lights.updateBuffer();
}

template<LightType T> TAGLightManager<T>::TAGLightManager(const TAGResourceManager::BufferAccess& access, const size_t size) : lights(size, shaderLightConverter, access, true) {}

template<LightType T> void TAGLightManager<T>::setLight(const T& light, const int& index) {
	if (index < 0) {
		lights.pushObject(light);
	}
	else if (index < lights.getAllObjects().size()) {
		lights.setObject(light, index);
	}
}

template<LightType T> template<typename V> requires VariableLightMemberName::Concept<T, V> void TAGLightManager<T>::setLightMember(const V::TYPE& value, const GLuint& index) {
	lights.setObjectMember<typename V::TYPE>(value, V::OFFSET, index);
}

template<LightType T> T TAGLightManager<T>::removeLight(const int& index) {
	return (index < 0 ? lights.popObject() : lights.removeObject(index));
}

template<LightType T> void TAGLightManager<T>::setAllLights(const std::vector<T>& lights) {
	this->lights.setAllObjects(lights);
}

template<LightType T> const T& TAGLightManager<T>::getLight(const int& index) const {
	return (index < 0 ? lights.peekObject() : lights.getObject(index)) ;
}

template<LightType T> const std::vector<T>& TAGLightManager<T>::getAllLights() const {
	return lights.getAllObjects();
}

template<LightType T> void TAGLightManager<T>::bindToShader(const GLintptr& offset, const GLuint& index) {
	lights.bindToShader(index, offset, TAGResourceManager::ShaderBufferType::SHADER_STORAGE);
}

template<LightType T> void TAGLightManager<T>::updateLightBuffer() {
	lights.updateBuffer();
}

template<LightType T> auto TAGLightManager<T>::begin() const {
	return lights.begin();
}

template<LightType T> auto TAGLightManager<T>::end() const {
	return lights.end();
}

template<LightType T> unsigned int TAGLightManager<T>::bufferSize() const {
	return lights.getBuffer()->getCurrentObjects();
}

template<LightType T> std::array<TAGLightManager<T>::ShaderT, 1> TAGLightManager<T>::shaderLightConverter(const T& light) {
	if constexpr (std::same_as<T, TAGLight::Point>) {
		return { { glm::vec4(light.position, light.attenuation.x), glm::vec4(light.colour, light.attenuation.y) } };
	}
	else if constexpr (std::same_as<T, TAGLight::Ray>) {
		return { { glm::vec4(light.direction, light.colour.x), glm::vec2(light.colour.y, light.colour.z) } };
	}
	else {
		return { { glm::vec4(light.position, light.attenuation.x), glm::vec4(light.direction, light.attenuation.y), glm::vec4(light.colour, light.angle) } };
	}
}
