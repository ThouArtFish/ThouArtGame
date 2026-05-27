#pragma once

#include <LightManagerClass.hpp>

template<LightType T> TAGLightManager<T>::TAGLightManager(const std::vector<T>& lights, const TAGResourceManager::BufferAccess& access, const unsigned int& size) {
	if (std::holds_alternative<std::monostate>(scene)) {
		initSceneBuffer();
	}
	this->lights = TAGResourceManager::ObjectBuffer<T, ShaderT>(glm::max(size, lights.size()), shaderLightConverter, access, true);
	this->lights.setAllObjects(lights);
	this->lights.updateBuffer();
}

template<LightType T> TAGLightManager<T>::TAGLightManager(const TAGResourceManager::BufferAccess& access, const unsigned int& size) : lights(size, shaderLightConverter, access, true) {
	if (std::holds_alternative<std::monostate>(scene)) {
		initSceneBuffer();
	}
}

template<LightType T> void TAGLightManager<T>::setLight(const T& light, const int& index) {
	if (index < 0) {
		lights.pushObject(light);
	}
	else if (index < lights.getAllObjects().size()) {
		lights.setObject(light, index);
	}
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
	if (lights.isObjectsChanged()) {
		updateLightBuffer();
	}
	lights.getBuffer()->bindToShader(index, offset, TAGResourceManager::ShaderBufferType::SHADER_STORAGE);
}

template<LightType T> void TAGLightManager<T>::updateLightBuffer() {
	lights.updateBuffer();
}

template<LightType T> void TAGLightManager<T>::setScene(const Scene& scene) {
	SceneObject* scene_ptr = std::get_if<SceneObject>(scene);
	if (scene_ptr) {
		scene_ptr->setObject(scene);
		scene_ptr->updateBuffer();
	}
}

template<LightType T> const TAGLightManager<T>::Scene& TAGLightManager<T>::getScene() {
	return std::get_if<SceneObject>(scene)->peekObject();
}

template<LightType T> void TAGLightManager<T>::bindSceneToShader(const GLuint& index) {
	SceneObject* scene_ptr = std::get_if<SceneObject>(scene);
	if (scene_ptr) {
		scene_ptr->getBuffer()->bindToShader(index, 0, TAGResourceManager::ShaderBufferType::SHADER_STORAGE);
	}
}

template<LightType T> auto TAGLightManager<T>::begin() const {
	return lights.getAllObjects().begin();
}

template<LightType T> auto TAGLightManager<T>::end() const {
	return lights.getAllObjects().end();
}

template<LightType T> unsigned int TAGLightManager<T>::bufferSize() const {
	return lights.getBuffer()->getCurrentObjects();
}

template<LightType T> TAGLightManager<T>::ShaderT TAGLightManager<T>::shaderLightConverter(const T& light, const GLuint& split) {
	if constexpr (std::same_as<T, PointLight>) {
		return { glm::vec4(light.position, light.attenuation.x), glm::vec4(light.colour, light.attenuation.y) };
	}
	else if constexpr (std::same_as<T, RayLight>) {
		return { glm::vec4(light.direction, light.colour.x), glm::vec2(light.colour.y, light.colour.z) };
	}
	else {
		return { glm::vec4(light.position, light.attenuation.x), glm::vec4(light.direction, light.attenuation.y), glm::vec4(light.colour, light.angle) };
	}
}

template<LightType T> GLfloat TAGLightManager<T>::shaderSceneConverter(const Scene& scene, const GLuint& split) {
	return scene.ambience;
}

template<LightType T> void TAGLightManager<T>::initSceneBuffer() {
	scene.emplace<SceneObject>(1, shaderSceneConverter, TAGResourceManager::BufferAccess::STATIC);
	SceneObject& scene_buffer = std::get<1>(scene);
	scene_buffer.pushObject({ 0.1f });
	scene_buffer.updateBuffer();
}
