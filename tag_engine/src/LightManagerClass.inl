#pragma once

#include <LightManagerClass.hpp>

template<LightType T> TAGLightManager<T>::TAGLightManager(const std::vector<T>& lights, const TAGResourceManager::BufferAccess& access, const unsigned int& size) {
	if (std::holds_alternative<std::monostate>(scene)) {
		initSceneBuffer();
	}
	this->lights = TAGResourceManager::ObjectBuffer<T, ShaderT>(glm::max(size, lights.size()), shaderLightConverter<T>, access);
	std::vector<T>& light_ref = this->lights.changeObjects();
	light_ref = lights;
	this->lights.updateBuffer();
}

template<LightType T> TAGLightManager<T>::TAGLightManager(const TAGResourceManager::BufferAccess& access, const unsigned int& size) {
	if (std::holds_alternative<std::monostate>(scene)) {
		initSceneBuffer();
	}
	lights = TAGResourceManager::ObjectBuffer<T, ShaderT>(size, shaderLightConverter<T>, access);
}

template<LightType T> void TAGLightManager<T>::setLight(const T& light, const int& index) {
	std::vector<T>& light_array = lights.changeObjects();
	if (index < 0) {
		light_array.push_back(light);
	}
	else {
		light_array[index] = light;
	}
}

template<LightType T> const T& TAGLightManager<T>::getLight(const int& index) const {
	const std::vector<T>& light_array = lights.getObjects();
	return (index < 0 ? light_array.back() : light_array[index]);
}

template<LightType T> const std::vector<T>& TAGLightManager<T>::getAllLights() const {
	return lights.getObjects();
}

template<LightType T> void TAGLightManager<T>::bindToShader(const GLintptr& offset, const GLuint& index) {
	if (lights.isObjectsChanged()) {
		updateLightBuffer(index);
	}
	lights.getBuffer()->bindToShader(index, offset, TAGResourceManager::ShaderBuffer::SHADER_STORAGE);
}

template<LightType T> void TAGLightManager<T>::updateLightBuffer(const GLuint& index) {
	lights.updateBuffer();
	setLightCount(index);
}

template<LightType T> void TAGLightManager<T>::setAmbience(const float& ambience) {
	SceneObject* scene_ptr = std::get_if<SceneObject>(scene);
	if (scene_ptr) {
		scene_ptr->changeObjects()[0].ambience = ambience;
	}
}

template<LightType T> float TAGLightManager<T>::getAmbience() {
	SceneObject* scene_ptr = std::get_if<SceneObject>(scene);
	if (scene_ptr) {
		return scene_ptr->getObjects()[0].ambience;
	}
	return -1.0f;
}

template<LightType T> void TAGLightManager<T>::bindSceneToShader(const GLuint& index) {
	SceneObject* scene_ptr = std::get_if<SceneObject>(scene);
	if (scene_ptr) {
		if (scene_ptr->isObjectsChanged()) {
			scene_ptr->updateBuffer();
		}
		scene_ptr->getBuffer()->bindToShader(index, 0, TAGResourceManager::ShaderBuffer::SHADER_STORAGE);
	}
}

template<LightType T> void TAGLightManager<T>::updateSceneBuffer() {
	SceneObject* scene_ptr = std::get_if<SceneObject>(scene);
	if (scene_ptr) {
		scene_ptr->updateBuffer();
	}
}

template<LightType T> auto TAGLightManager<T>::begin() const {
	return lights.getObjects().begin();
}

template<LightType T> auto TAGLightManager<T>::end() const {
	return lights.getObjects().end();
}

template<LightType T> unsigned int TAGLightManager<T>::bufferSize() const {
	return lights.getBuffer()->getCurrentObjects();
}

template<LightType T> unsigned int TAGLightManager<T>::size() const {
	return lights.getObjects().size();
}

template<LightType T> void TAGLightManager<T>::setLightCount(const GLuint& index) {
	const int current_object_num = lights.getBuffer()->getCurrentObjects();
	SceneObject& scene_obj = std::get<SceneObject>(scene);
	scene_obj.changeObjects()[0].light_counts[index] = current_object_num;
	scene_obj.updateBuffer();
	last_size = current_object_num;
}

template<LightType T> TAGLightManager<T>::ShaderT TAGLightManager<T>::shaderLightConverter(const T& light, const unsigned int& split) {
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

template<LightType T> GLfloat TAGLightManager<T>::shaderSceneConverter(const Scene& scene, const unsigned int& split) {
	if (split < MAX_BINDING_INDEX + 2) {
		return (GLfloat)scene.light_counts[split];
	}
	return scene.ambience;
}

template<LightType T> void TAGLightManager<T>::initSceneBuffer() {
	scene.emplace<SceneObject>(1, shaderSceneConverter<T>, TAGResourceManager::BufferAccess::STREAM);
	SceneObject& scene_buffer = std::get<1>(scene);
	Scene new_scene;
	for (unsigned int i = 0; i < MAX_BINDING_INDEX; i++) {
		new_scene.binding_and_counts[i] = { i, 0 };
	}
	new_scene.ambience = 0.1f;
	scene_buffer.changeObjects().push_back(new_scene);
	scene_buffer.updateBuffer();
}
